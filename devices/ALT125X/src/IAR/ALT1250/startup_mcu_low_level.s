        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION CSTACK:DATA:NOROOT(3)

        SECTION .intvec:CODE:NOROOT(2)

        EXTERN  __iar_program_start
        EXTERN  SystemInit
        PUBLIC  __vector_table
        PUBLIC  __Vectors
        PUBLIC  __Vectors_End
        PUBLIC  __Vectors_Size

        DATA
__vector_table

                DCD     sfe(CSTACK)
                DCD     Reset_Handler             ; Reset Handler
                DCD     NMI_Handler               ; NMI Handler
                DCD     HardFault_Handler         ; Hard Fault Handler
                DCD     MemManage_Handler         ; MPU Fault Handler
                DCD     BusFault_Handler          ; Bus Fault Handler
                DCD     UsageFault_Handler        ; Usage Fault Handler
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     0                         ; Reserved
                DCD     SVC_Handler               ; SVCall Handler
                DCD     DebugMon_Handler          ; Debug Monitor Handler
                DCD     0                         ; Reserved
                DCD     PendSV_Handler            ; PendSV Handler
                DCD     SysTick_Handler           ; SysTick Handler

                ; External Interrupts
                DCD     NULL_IRQHandler           ; 0: Reserved
                DCD     NULL_IRQHandler           ; 1: Reserved
                DCD     auxdac0_handler           ; 2: Aux ADC int 0
                DCD     auxdac1_handler           ; 3: Aux ADC int 1
                DCD     auxdac2_handler           ; 4: Aux ADC int 2
                DCD     auxdac3_handler           ; 5: Aux ADC int 3
                DCD     auxdac4_handler           ; 6: Aux ADC int 4
                DCD     auxdac5_handler           ; 7: Aux ADC int 5
                DCD     auxdac6_handler           ; 8: Aux ADC int 6
                DCD     auxdac7_handler           ; 9: Aux ADC int 7
                DCD     gdma_channel_0_handler    ; 10: GDMA channel 0 done
                DCD     gdma_channel_1_handler    ; 11: GDMA channel 1 done
                DCD     gdma_channel_2_handler    ; 12: GDMA channel 2 done
                DCD     gdma_channel_3_handler    ; 13: GDMA channel 3 done
                DCD     gdma_channel_4_handler    ; 14: GDMA channel 4 done
                DCD     gdma_channel_5_handler    ; 15: GDMA channel 5 done
                DCD     gdma_channel_6_handler    ; 16: GDMA channel 6 done
                DCD     gdma_channel_7_handler    ; 17: GDMA channel 7 done
                DCD     NULL_IRQHandler           ; 18: Reserved
                DCD     NULL_IRQHandler           ; 19: Reserved
                DCD     NULL_IRQHandler           ; 20: Reserved
                DCD     NULL_IRQHandler           ; 21: Reserved
                DCD     hw_uart_interrupt_handler0; 22: MCU_SS_UARTF0_INT
                DCD     UARTF_ERR_IRQHandler      ; 23: MCU_SS_UARTF0_ERR_INT
                DCD     hw_uart_interrupt_handler1; 24: MCU_SS_UARTF1_INT
                DCD     UARTF_ERR_IRQHandler      ; 25: MCU_SS_UARTF0_ERR_INT
                DCD     spis0_interrupt_handler   ; 26: MCU_SS_SPI_SLAVE_INT
                DCD     NULL_IRQHandler           ; 27: Reserved
                DCD     NULL_IRQHandler           ; 28: Reserved
                DCD     i2c0_interrupt_handler    ; 29: MCU_SS_I2C0_INTERRUPT
                DCD     NULL_IRQHandler           ; 30: Reserved
                DCD     i2c1_interrupt_handler    ; 31: MCU_SS_I2C1_INTERRUPT
                DCD     NULL_IRQHandler           ; 32: Reserved
                DCD     spim0_interrupt_handler   ; 33: MCU_SPIM0_Interrupt
                DCD     NULL_IRQHandler           ; 34: Reserved
                DCD     spim1_interrupt_handler   ; 35: MCU_SPIM1_Interrupt
                DCD     hw_uart_interrupt_handleri0 ; 36: MCU_SS_UARTI0_INTR
                DCD     UARTI_IRQHandler          ; 37: MCU_SS_UARTI1_INTR
                DCD     UARTI_IRQHandler          ; 38: MCU_SS_UARTI2_INTR
                DCD     NULL_IRQHandler           ; 39: Reserved
                DCD     NULL_IRQHandler           ; 40: Reserved
                DCD     gpio_0_handler            ; 41: Reserved
                DCD     gpio_1_handler            ; 42: Reserved
                DCD     gpio_2_handler            ; 43: Reserved
                DCD     gpio_3_handler            ; 44: Reserved
                DCD     gpio_4_handler            ; 45: Reserved
                DCD     gpio_5_handler            ; 46: Reserved
                DCD     gpio_6_handler            ; 47: Reserved
                DCD     gpio_7_handler            ; 48: Reserved
                DCD     gpio_8_handler            ; 49: Reserved
                DCD     gpio_9_handler            ; 50: Reserved
                DCD     GP_Timer_0_IRQHandler     ; 51: MCU GP Timer 0
                DCD     GP_Timer_1_IRQHandler     ; 52: MCU GP Timer 1
                DCD     shadow32ktimer_IRQhandler ; 53: MCU Shadow 32K Timer
                DCD     NULL_IRQHandler           ; 54: Reserved
                DCD     MCU_WD0_IRQHandler        ; 55: Reserved
                DCD     gpio_wakeup_IRQHandler    ; 56: GPM_IO_WAKEUP_INT_MCU
                DCD     ATMCTR_MAILBOX_IRQhandler ; 57: Reserved
                DCD     NULL_IRQHandler           ; 58: Reserved
                DCD     NULL_IRQHandler           ; 59: Reserved
                DCD     NULL_IRQHandler           ; 60: Reserved
                DCD     NULL_IRQHandler           ; 61: Reserved
                DCD     NULL_IRQHandler           ; 62: Reserved
                DCD     NULL_IRQHandler           ; 63: Reserved
__Vectors_End

__Vectors       EQU   __vector_table
__Vectors_Size  EQU   __Vectors_End - __Vectors

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Default interrupt handlers.
;;
        THUMB
        PUBWEAK Reset_Handler
        SECTION UNCACHED:CODE:NOROOT:REORDER(2)

Reset_Handler
        LDR     R0, =SystemInit
        BLX     R0
        LDR     R0, =__iar_program_start
        BX      R0

; Dummy Exception Handlers (infinite loops which can be modified)

        PUBWEAK NMI_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)

NMI_Handler     
                B       .


        PUBWEAK HardFault_Handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
HardFault_Handler
                B        .

        PUBWEAK MemManage_Handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
MemManage_Handler
                B        .

        PUBWEAK BusFault_Handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
BusFault_Handler
                B        .

        PUBWEAK UsageFault_Handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
UsageFault_Handler
                B        .

        PUBWEAK SVC_Handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
SVC_Handler
                B        .

        PUBWEAK DebugMon_Handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
DebugMon_Handler
                B        .

        PUBWEAK PendSV_Handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
PendSV_Handler
                B        .

        PUBWEAK SysTick_Handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
SysTick_Handler
                B        .

        PUBWEAK auxdac0_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
auxdac0_handler
                B        .

        PUBWEAK auxdac1_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
auxdac1_handler
                B        .

        PUBWEAK auxdac2_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
auxdac2_handler
                B        .

        PUBWEAK auxdac3_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
auxdac3_handler
                B        .

        PUBWEAK auxdac4_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
auxdac4_handler
                B        .

        PUBWEAK auxdac5_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
auxdac5_handler
                B        .

        PUBWEAK auxdac6_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
auxdac6_handler
                B        .

        PUBWEAK auxdac7_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
auxdac7_handler
                B        .

        PUBWEAK gdma_channel_0_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gdma_channel_0_handler
                B        .

        PUBWEAK gdma_channel_1_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gdma_channel_1_handler
                B        .
                
        PUBWEAK gdma_channel_2_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gdma_channel_2_handler
                B        .

        PUBWEAK gdma_channel_3_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gdma_channel_3_handler
                B        .

        PUBWEAK gdma_channel_4_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gdma_channel_4_handler
                B        .

        PUBWEAK gdma_channel_5_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gdma_channel_5_handler
                B        .

        PUBWEAK gdma_channel_6_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gdma_channel_6_handler
                B        .
                
        PUBWEAK gdma_channel_7_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gdma_channel_7_handler
                B        .

        PUBWEAK UARTI_IRQHandler
;        SECTION .text:CODE:NOROOT:REORDER(1)
UARTI_IRQHandler
                B        .

        PUBWEAK spis0_interrupt_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
spis0_interrupt_handler
                B        .

        PUBWEAK i2c0_interrupt_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
i2c0_interrupt_handler
                B        .

        PUBWEAK i2c1_interrupt_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
i2c1_interrupt_handler
                B        .

        PUBWEAK hw_uart_interrupt_handler0
;        SECTION .text:CODE:NOROOT:REORDER(1)
hw_uart_interrupt_handler0
                B        .
                
        PUBWEAK hw_uart_interrupt_handler1
;        SECTION .text:CODE:NOROOT:REORDER(1)
hw_uart_interrupt_handler1
                B        .

        PUBWEAK hw_uart_interrupt_handleri0
;        SECTION .text:CODE:NOROOT:REORDER(1)
hw_uart_interrupt_handleri0
                B        .

        PUBWEAK UARTF_ERR_IRQHandler
;        SECTION .text:CODE:NOROOT:REORDER(1)
UARTF_ERR_IRQHandler
                B        .

        PUBWEAK gpio_0_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gpio_0_handler
                B        .

        PUBWEAK gpio_1_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gpio_1_handler
                B        .

        PUBWEAK gpio_2_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gpio_2_handler
                B        .

        PUBWEAK gpio_3_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gpio_3_handler
                B        .

        PUBWEAK gpio_4_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gpio_4_handler
                B        .

        PUBWEAK gpio_5_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gpio_5_handler
                B        .

        PUBWEAK gpio_6_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gpio_6_handler
                B        .

        PUBWEAK gpio_7_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gpio_7_handler
                B        .

        PUBWEAK gpio_8_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gpio_8_handler
                B        .

        PUBWEAK gpio_9_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gpio_9_handler
                B        .

        PUBWEAK GP_Timer_0_IRQHandler
;        SECTION .text:CODE:NOROOT:REORDER(1)
GP_Timer_0_IRQHandler
                B        .

        PUBWEAK GP_Timer_1_IRQHandler
;        SECTION .text:CODE:NOROOT:REORDER(1)
GP_Timer_1_IRQHandler
                B        .
                
        PUBWEAK shadow32ktimer_IRQhandler
;        SECTION .text:CODE:NOROOT:REORDER(1)
shadow32ktimer_IRQhandler
                B        .

        PUBWEAK spim0_interrupt_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
spim0_interrupt_handler
                B        .

        PUBWEAK spim1_interrupt_handler
;        SECTION .text:CODE:NOROOT:REORDER(1)
spim1_interrupt_handler
                B        .

        PUBWEAK MCU_WD0_IRQHandler
;        SECTION .text:CODE:NOROOT:REORDER(1)
MCU_WD0_IRQHandler
                B        .                

        PUBWEAK gpio_wakeup_IRQHandler
;        SECTION .text:CODE:NOROOT:REORDER(1)
gpio_wakeup_IRQHandler
                B        .                

        PUBWEAK ATMCTR_MAILBOX_IRQhandler
;        SECTION .text:CODE:NOROOT:REORDER(1)
ATMCTR_MAILBOX_IRQhandler
                B        .

        PUBWEAK NULL_IRQHandler
;        SECTION .text:CODE:NOROOT:REORDER(1)
NULL_IRQHandler
                B        .

        END
