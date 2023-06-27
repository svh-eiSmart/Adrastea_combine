/****************************************************************************
 *
 *  (c) copyright 2020 Altair Semiconductor, Ltd. All rights reserved.
 *
 *  This software, in source or object form (the "Software"), is the
 *  property of Altair Semiconductor Ltd. (the "Company") and/or its
 *  licensors, which have all right, title and interest therein, You
 *  may use the Software only in  accordance with the terms of written
 *  license agreement between you and the Company (the "License").
 *  Except as expressly stated in the License, the Company grants no
 *  licenses by implication, estoppel, or otherwise. If you are not
 *  aware of or do not agree to the License terms, you may not use,
 *  copy or modify the Software. You may use the source code of the
 *  Software only for your internal purposes and may not distribute the
 *  source code of the Software, any part thereof, or any derivative work
 *  thereof, to any third party, except pursuant to the Company's prior
 *  written consent.
 *  The Software is the confidential information of the Company.
 *
 ****************************************************************************/

#include "FreeRTOS.h"
#if ( configUSE_ALT1250_SLEEP == 1 )
#include "pwr_mngr.h"
#include "shadow32ktimer.h"
#endif
/* For backward compatibility, ensure configKERNEL_INTERRUPT_PRIORITY is
defined.  The value should also ensure backward compatibility.
FreeRTOS.org versions prior to V4.4.0 did not include this definition. */
#ifndef configKERNEL_INTERRUPT_PRIORITY
	#define configKERNEL_INTERRUPT_PRIORITY 255
#endif

#ifndef configSYSTICK_CLOCK_HZ
	#define configSYSTICK_CLOCK_HZ configCPU_CLOCK_HZ
	/* Ensure the SysTick is clocked at the same frequency as the core. */
	#define portNVIC_SYSTICK_CLK_BIT	( 1UL << 2UL )
#else
	/* The way the SysTick is clocked is not modified in case it is not the same
	as the core. */
	#define portNVIC_SYSTICK_CLK_BIT	( 0 )
#endif

/* Constants required to manipulate the core.  Registers first... */
#define portNVIC_SYSTICK_CTRL_REG			( * ( ( volatile uint32_t * ) 0xe000e010 ) )
#define portNVIC_SYSTICK_LOAD_REG			( * ( ( volatile uint32_t * ) 0xe000e014 ) )
#define portNVIC_SYSTICK_CURRENT_VALUE_REG	( * ( ( volatile uint32_t * ) 0xe000e018 ) )
#define portNVIC_SYSPRI2_REG				( * ( ( volatile uint32_t * ) 0xe000ed20 ) )
/* ...then bits in the registers. */
#define portNVIC_SYSTICK_INT_BIT			( 1UL << 1UL )
#define portNVIC_SYSTICK_ENABLE_BIT			( 1UL << 0UL )
#define portNVIC_SYSTICK_COUNT_FLAG_BIT		( 1UL << 16UL )
#define portNVIC_PENDSVCLEAR_BIT 			( 1UL << 27UL )
#define portNVIC_PEND_SYSTICK_CLEAR_BIT		( 1UL << 25UL )

#define portNVIC_PENDSV_PRI					( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 16UL )
#define portNVIC_SYSTICK_PRI				( ( ( uint32_t ) configKERNEL_INTERRUPT_PRIORITY ) << 24UL )

/* Constants required to check the validity of an interrupt priority. */
#define portFIRST_USER_INTERRUPT_NUMBER		( 16 )
#define portNVIC_IP_REGISTERS_OFFSET_16 	( 0xE000E3F0 )
#define portAIRCR_REG						( * ( ( volatile uint32_t * ) 0xE000ED0C ) )
#define portMAX_8_BIT_VALUE					( ( uint8_t ) 0xff )
#define portTOP_BIT_OF_BYTE					( ( uint8_t ) 0x80 )
#define portMAX_PRIGROUP_BITS				( ( uint8_t ) 7 )
#define portPRIORITY_GROUP_MASK				( 0x07UL << 8UL )
#define portPRIGROUP_SHIFT					( 8UL )

/* Masks off all bits but the VECTACTIVE bits in the ICSR register. */
#define portVECTACTIVE_MASK					( 0xFFUL )

/* Constants required to set up the initial stack. */
#define portINITIAL_XPSR					( 0x01000000UL )

/* The systick is a 24-bit counter. */
#define portMAX_24_BIT_NUMBER				( 0xffffffUL )

/* A fiddle factor to estimate the number of SysTick counts that would have
occurred while the SysTick counter is stopped during tickless idle
calculations. */
#define portMISSED_COUNTS_FACTOR			( 45UL )

/*
 * The number of SysTick increments that make up one tick period.
 */
#if( configUSE_TICKLESS_IDLE == 1 )
	static uint32_t ulTimerCountsForOneTick = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * The maximum number of tick periods that can be suppressed is limited by the
 * 24 bit resolution of the SysTick timer.
 */
#if( configUSE_TICKLESS_IDLE == 1 )
	static uint32_t xMaximumPossibleSuppressedTicks = 0;
#endif /* configUSE_TICKLESS_IDLE */

/*
 * Compensate for the CPU cycles that pass while the SysTick is stopped (low
 * power functionality only.
 */
#if( configUSE_TICKLESS_IDLE == 1 )
	static uint32_t ulStoppedTimerCompensation = 0;
#endif /* configUSE_TICKLESS_IDLE */

static volatile int64_t x64TickCnt = ( int64_t ) configINITIAL_TICK_COUNT;
static volatile int64_t x64PrevUpdateTickCnt = ( int64_t ) configINITIAL_TICK_COUNT;

#if( configUSE_TICKLESS_IDLE == 1 )

	void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
	{
        uint32_t ulReloadValue, ulCompleteTickPeriods, ulCompletedSysTickDecrements;
        TickType_t xModifiableIdleTime, ExpectedStopTime = xExpectedIdleTime;

#if ( configUSE_ALT1250_SLEEP == 1 )
        power_mode_e pwr_mode = pwr_conf_param_get_mode();
		if(pwr_mode == PWR_MODE_RUN)
			return;

        if (pwr_mode != PWR_MODE_SLEEP) {
            if(pwr_check_allow2sleep() == 0) {
                pwr_mode = PWR_MODE_SLEEP; /* graceful downgrade to default(sleep) mode */
            }
        }
#endif
		/* Make sure the SysTick reload value does not overflow the counter. */
		if( xExpectedIdleTime > xMaximumPossibleSuppressedTicks )
		{
			xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
		}

		/* Stop the SysTick momentarily.  The time the SysTick is stopped for
		is accounted for as best it can be, but using the tickless mode will
		inevitably result in some tiny drift of the time maintained by the
		kernel with respect to calendar time. */
		portNVIC_SYSTICK_CTRL_REG &= ~portNVIC_SYSTICK_ENABLE_BIT;

		/* Calculate the reload value required to wait xExpectedIdleTime
		tick periods.  -1 is used because this code will execute part way
		through one of the tick periods. */
		ulReloadValue = portNVIC_SYSTICK_CURRENT_VALUE_REG + ( ulTimerCountsForOneTick * ( xExpectedIdleTime - 1UL ) );
		if( ulReloadValue > ulStoppedTimerCompensation )
		{
			ulReloadValue -= ulStoppedTimerCompensation;
		}

		/* Enter a critical section but don't use the taskENTER_CRITICAL()
		method as that will mask interrupts that should exit sleep mode. */
		__asm volatile( "cpsid i" ::: "memory" );
		__asm volatile( "dsb" );
		__asm volatile( "isb" );
#if ( configUSE_ALT1250_SLEEP == 1 )
		if (pwr_mode != PWR_MODE_SLEEP)
        	pwr_mask_interrupts(pwr_mode);
#endif
		/* If a context switch is pending or a task is waiting for the scheduler
		to be unsuspended then abandon the low power entry. */
		if( eTaskConfirmSleepModeStatus() == eAbortSleep )
		{
			/* Restart from whatever is left in the count register to complete
			this tick period. */
			portNVIC_SYSTICK_LOAD_REG = portNVIC_SYSTICK_CURRENT_VALUE_REG;

			/* Restart SysTick. */
			portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

			/* Reset the reload register to the value required for normal tick
			periods. */
			portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;

			/* Re-enable interrupts - see comments above the cpsid instruction()
			above. */
#if ( configUSE_ALT1250_SLEEP == 1 )
			if (pwr_mode != PWR_MODE_SLEEP)
            	pwr_restore_interrupts();
#endif
			__asm volatile( "cpsie i" ::: "memory" );
		}
		else
		{
#if ( configUSE_ALT1250_SLEEP == 1 )
            if( pwr_mode == PWR_MODE_SLEEP )
#endif
            {
                /* Set the new reload value. */
                portNVIC_SYSTICK_LOAD_REG = ulReloadValue;

                /* Clear the SysTick count flag and set the count value back to
                zero. */
                portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

                /* Restart SysTick. */
                portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;
            }

			/* Sleep until something happens.  configPRE_SLEEP_PROCESSING() can
			set its parameter to 0 to indicate that its implementation contains
			its own wait for interrupt or wait for event instruction, and so wfi
			should not be executed again.  However, the original expected idle
			time variable must remain unmodified, so a copy is taken. */
			xModifiableIdleTime = xExpectedIdleTime;
			configPRE_SLEEP_PROCESSING( xModifiableIdleTime );
#if ( configUSE_ALT1250_SLEEP == 1 )
            if( pwr_mode != PWR_MODE_SLEEP ) {
                pwr_pre_sleep_process(xModifiableIdleTime);
            }

            pwr_gatedclks_handle_before_sleep();
#endif
			if( xModifiableIdleTime > 0 )
			{
#if ( configUSE_ALT1250_SLEEP == 1 )
                if( pwr_mode != PWR_MODE_SLEEP ) {
					SCB->ICSR = SCB->ICSR | (0x8000000); //clear pendSV status
					if(pwr_mode == PWR_MODE_STOP) {
						//start
						shadow32ktimer_activate(0);
						if(ExpectedStopTime > PWR_DEF_MAX_DRUATION){
							ExpectedStopTime = PWR_DEF_MAX_DRUATION;					
						}
						pwr_conf_param_set(PWR_MODE_STOP, ExpectedStopTime);
						xExpectedIdleTime = ExpectedStopTime;
					}
                    PWR_EnterSleep();
                }
                else 
#endif
                {
                    __asm volatile( "dsb" ::: "memory" );
                    __asm volatile( "wfi" );
                    __asm volatile( "isb" );
                }
			}
#if ( configUSE_ALT1250_SLEEP == 1 )
            pwr_gatedclks_handle_after_sleep();
            if( pwr_mode != PWR_MODE_SLEEP ) {
                pwr_post_sleep_process( xModifiableIdleTime );
            }
#endif
			configPOST_SLEEP_PROCESSING( xExpectedIdleTime );
#if ( configUSE_ALT1250_SLEEP == 1 )
			if(pwr_mode == PWR_MODE_STOP) {
				//stop
				shadow32ktimer_disable();
				shadow32ktimer_disable_interrupt();
			}
#endif
			/* Re-enable interrupts to allow the interrupt that brought the MCU
			out of sleep mode to execute immediately.  see comments above
			__disable_interrupt() call above. */
			__asm volatile( "cpsie i" ::: "memory" );
			__asm volatile( "dsb" );
			__asm volatile( "isb" );
#if ( configUSE_ALT1250_SLEEP == 1 )
			if (pwr_mode != PWR_MODE_SLEEP)
            	pwr_restore_interrupts();
#endif
			/* Disable interrupts again because the clock is about to be stopped
			and interrupts that execute while the clock is stopped will increase
			any slippage between the time maintained by the RTOS and calendar
			time. */
			__asm volatile( "cpsid i" ::: "memory" );
			__asm volatile( "dsb" );
			__asm volatile( "isb" );

#if ( configUSE_ALT1250_SLEEP == 1 )
			if (pwr_mode != PWR_MODE_SLEEP)
				pwr_mask_interrupts(pwr_mode);
#endif

#if ( configUSE_ALT1250_SLEEP == 1 )
			if(pwr_mode == PWR_MODE_STOP) {
				pwr_wakeup_stat_t statistics;
				if(pwr_stat_get_sleep_statistics(&statistics) == 0) {
					if(xExpectedIdleTime > statistics.last_dur_left){
						xExpectedIdleTime = xExpectedIdleTime - statistics.last_dur_left;
					}
					else {
						xExpectedIdleTime = (shadow32ktimer_get_value()*1000+16384)/32768;
					}
				}
			}
#endif

			/* Disable the SysTick clock without reading the
			portNVIC_SYSTICK_CTRL_REG register to ensure the
			portNVIC_SYSTICK_COUNT_FLAG_BIT is not cleared if it is set.  Again,
			the time the SysTick is stopped for is accounted for as best it can
			be, but using the tickless mode will inevitably result in some tiny
			drift of the time maintained by the kernel with respect to calendar
			time*/
			portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT );

			/* Determine if the SysTick clock has already counted to zero and
			been set back to the current reload value (the reload back being
			correct for the entire expected idle time) or if the SysTick is yet
			to count to zero (in which case an interrupt other than the SysTick
			must have brought the system out of sleep mode). */
			if( ( portNVIC_SYSTICK_CTRL_REG & portNVIC_SYSTICK_COUNT_FLAG_BIT ) != 0 )
			{
				uint32_t ulCalculatedLoadValue;

				/* The tick interrupt is already pending, and the SysTick count
				reloaded with ulReloadValue.  Reset the
				portNVIC_SYSTICK_LOAD_REG with whatever remains of this tick
				period. */
				ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL ) - ( ulReloadValue - portNVIC_SYSTICK_CURRENT_VALUE_REG );

				/* Don't allow a tiny value, or values that have somehow
				underflowed because the post sleep hook did something
				that took too long. */
				if( ( ulCalculatedLoadValue < ulStoppedTimerCompensation ) || ( ulCalculatedLoadValue > ulTimerCountsForOneTick ) )
				{
					ulCalculatedLoadValue = ( ulTimerCountsForOneTick - 1UL );
				}

				portNVIC_SYSTICK_LOAD_REG = ulCalculatedLoadValue;

				/* As the pending tick will be processed as soon as this
				function exits, the tick value maintained by the tick is stepped
				forward by one less than the time spent waiting. */
				ulCompleteTickPeriods = xExpectedIdleTime - 1UL;
			}
			else
			{
				/* Something other than the tick interrupt ended the sleep.
				Work out how long the sleep lasted rounded to complete tick
				periods (not the ulReload value which accounted for part
				ticks). */
				ulCompletedSysTickDecrements = ( xExpectedIdleTime * ulTimerCountsForOneTick ) - portNVIC_SYSTICK_CURRENT_VALUE_REG;

				/* How many complete tick periods passed while the processor
				was waiting? */
				ulCompleteTickPeriods = ulCompletedSysTickDecrements / ulTimerCountsForOneTick;

				/* The reload value is set to whatever fraction of a single tick
				period remains. */
				portNVIC_SYSTICK_LOAD_REG = ( ( ulCompleteTickPeriods + 1UL ) * ulTimerCountsForOneTick ) - ulCompletedSysTickDecrements;
			}

			/* Restart SysTick so it runs from portNVIC_SYSTICK_LOAD_REG
			again, then set portNVIC_SYSTICK_LOAD_REG back to its standard
			value. */
			portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;
			portNVIC_SYSTICK_CTRL_REG |= portNVIC_SYSTICK_ENABLE_BIT;

			vTaskStepTick( ulCompleteTickPeriods );

			int64_t x64TickCntDiff;

			x64TickCnt = ( INT64_MAX == x64TickCnt ) ? (0) : ( x64TickCnt + ulCompleteTickPeriods );
			x64TickCntDiff = ( x64TickCnt >= x64PrevUpdateTickCnt ) ? ( x64TickCnt - x64PrevUpdateTickCnt ) : ( INT64_MAX - x64PrevUpdateTickCnt + x64TickCnt );
			if (x64TickCntDiff > 1000 * portTICK_PERIOD_MS)
			{
				extern void update_system_time( int seconds );

				update_system_time( x64TickCntDiff/(1000 * portTICK_PERIOD_MS) );
				x64PrevUpdateTickCnt = ( ( INT64_MAX - x64TickCntDiff ) >= x64PrevUpdateTickCnt ) ? ( x64PrevUpdateTickCnt +  x64TickCntDiff ) : ( ( x64TickCntDiff - ( INT64_MAX - x64PrevUpdateTickCnt ) - 1 ) );
			}

			portNVIC_SYSTICK_LOAD_REG = ulTimerCountsForOneTick - 1UL;

			/* Exit with interrpts enabled. */
#if ( configUSE_ALT1250_SLEEP == 1 )
			if (pwr_mode != PWR_MODE_SLEEP) {
            	pwr_restore_interrupts();
				pwr_refresh_uart_inactive_time();
			}
#endif
			__asm volatile( "cpsie i" ::: "memory" );
		}
	}

#endif /* configUSE_TICKLESS_IDLE */
/*-----------------------------------------------------------*/

/*
 * Setup the systick timer to generate the tick interrupts at the required
 * frequency.
 */
void vPortSetupTimerInterrupt( void )
{
	/* Calculate the constants required to configure the tick interrupt. */
	#if( configUSE_TICKLESS_IDLE == 1 )
	{
		ulTimerCountsForOneTick = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ );
		xMaximumPossibleSuppressedTicks = portMAX_24_BIT_NUMBER / ulTimerCountsForOneTick;
		ulStoppedTimerCompensation = portMISSED_COUNTS_FACTOR / ( configCPU_CLOCK_HZ / configSYSTICK_CLOCK_HZ );
	}
	#endif /* configUSE_TICKLESS_IDLE */

	/* Stop and clear the SysTick. */
	portNVIC_SYSTICK_CTRL_REG = 0UL;
	portNVIC_SYSTICK_CURRENT_VALUE_REG = 0UL;

	/* Configure SysTick to interrupt at the requested rate. */
	portNVIC_SYSTICK_LOAD_REG = ( configSYSTICK_CLOCK_HZ / configTICK_RATE_HZ ) - 1UL;
	portNVIC_SYSTICK_CTRL_REG = ( portNVIC_SYSTICK_CLK_BIT | portNVIC_SYSTICK_INT_BIT | portNVIC_SYSTICK_ENABLE_BIT );
}
/*-----------------------------------------------------------*/
