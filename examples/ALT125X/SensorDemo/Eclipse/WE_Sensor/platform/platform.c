/**
 ***************************************************************************************************
 * This file is part of WE sensors SDK:
 * https://www.we-online.com/sensors, https://github.com/WurthElektronik/Sensors-SDK
 *
 * THE SOFTWARE INCLUDING THE SOURCE CODE IS PROVIDED â€œAS ISâ€�. YOU ACKNOWLEDGE THAT WÃœRTH ELEKTRONIK
 * EISOS MAKES NO REPRESENTATIONS AND WARRANTIES OF ANY KIND RELATED TO, BUT NOT LIMITED
 * TO THE NON-INFRINGEMENT OF THIRD PARTIESâ€™ INTELLECTUAL PROPERTY RIGHTS OR THE
 * MERCHANTABILITY OR FITNESS FOR YOUR INTENDED PURPOSE OR USAGE. WÃœRTH ELEKTRONIK EISOS DOES NOT
 * WARRANT OR REPRESENT THAT ANY LICENSE, EITHER EXPRESS OR IMPLIED, IS GRANTED UNDER ANY PATENT
 * RIGHT, COPYRIGHT, MASK WORK RIGHT, OR OTHER INTELLECTUAL PROPERTY RIGHT RELATING TO ANY
 * COMBINATION, MACHINE, OR PROCESS IN WHICH THE PRODUCT IS USED. INFORMATION PUBLISHED BY
 * WÃœRTH ELEKTRONIK EISOS REGARDING THIRD-PARTY PRODUCTS OR SERVICES DOES NOT CONSTITUTE A LICENSE
 * FROM WÃœRTH ELEKTRONIK EISOS TO USE SUCH PRODUCTS OR SERVICES OR A WARRANTY OR ENDORSEMENT
 * THEREOF
 *
 * THIS SOURCE CODE IS PROTECTED BY A LICENSE.
 * FOR MORE INFORMATION PLEASE CAREFULLY READ THE LICENSE AGREEMENT FILE LOCATED
 * IN THE ROOT DIRECTORY OF THIS DRIVER PACKAGE.
 *
 * COPYRIGHT (c) 2020 WÃ¼rth Elektronik eiSos GmbH & Co. KG
 *
 ***************************************************************************************************
 **/

#include "platform.h"
#include "i2c.h"
#include "spi.h"
#include "gpio.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>


#define DUMMY 0x00
/* I2C */
static uint32_t device_address;
static i2c_bus_e i2c_bus_id = I2C0_BUS;
static i2c_devid_mode_e i2c_opmode = I2C_DEVID_MODE_7BITS;

/* SPI */
static bool spi_is_configured = false;
spim_handle_t spim_handle;
static eMcuGpioIds SPI_SS_GPIO_ID = MCU_GPIO_ID_01;
static int8_t set_ss_active(int val);  // 0:de-active; otherwise, active
static int8_t spi_transfer_data(uint8_t *txData, uint8_t *rxData, uint8_t datalength);


/**
* @brief  Initialize the I2C Interface
* @param  I2C address
* @retval Error Code
*/
int8_t I2CInit(int address)
{
	if(0 != i2c_init(I2C0_BUS))
	{
		return WE_FAIL;
	}

	device_address = address;
	return WE_SUCCESS;
}

int8_t I2C_SetAddress(int address){
	device_address = address;
}

int8_t I2C_SetMode(int mode)
{
	return i2c_select_speed(i2c_bus_id, mode);
}

/**
* @brief   Read data starting from the addressed register
* @param  -RegAdr : the register address to read from
*         -NumByteToRead : number of bytes to read
*         -pointer Data : the address store the data
* @retval Error Code
*/
int8_t I2C_ReadReg(uint8_t RegAdr, int NumByteToRead, uint8_t *Data)
{
	if(NULL == Data)
	{
		return WE_FAIL;
	}
	//print("under i2c_readreg");

	if (i2c_read_advanced_mode(device_address, i2c_opmode, RegAdr, 1, NumByteToRead, Data, i2c_bus_id) != 0) {
		return WE_FAIL;
    }

  return WE_SUCCESS;
}

/**
* @brief  Write data starting from the addressed register
* @param  -RegAdr : Address to write in
*         -NumByteToWrite : number of bytes to write
*         -pointer Data : Address of the data to be written
* @retval Error Code
*/
int8_t I2C_WriteReg(uint8_t RegAdr, int NumByteToWrite, uint8_t *Data)
{
	if(NULL == Data)
	{
		return WE_FAIL;
	}

	if (i2c_write_advanced_mode(device_address, i2c_opmode, RegAdr, 1, NumByteToWrite, Data, i2c_bus_id) != 0) {
      return WE_FAIL;
    }

  return WE_SUCCESS;
}

int8_t SPI_Init()
{
  spi_config_t mConfig;
  int ret_val = 0;

  if(spi_is_configured == true) { return WE_SUCCESS; }

  gpio_init();

  if ((ret_val = spi_get_def_config(SPIM1_BUS, &mConfig)) != 0) {
    printf("SPI master: error to load default config from MCU wizard. error:%d!\r\n", ret_val);
    return WE_FAIL;
  }

   mConfig.param.ssSignalMgmt = SPI_SS_SIGNAL_BY_APP;

  /* spi master configuration */
  if ((ret_val = spi_configure(&mConfig, SPIM_SS_1)) != 0) {
    printf("SPI master-1: error during initialization. error:%d!\r\n", ret_val);
    return WE_FAIL;
  }

  /* 2. [optional] create handle and register callback function and event */
  spi_open_handle(SPIM1_BUS, &spim_handle, NULL, NULL);

  spi_is_configured = true;
  return 0;

	return WE_SUCCESS;
}

static int8_t set_ss_active(int val) {  // 0:de-active; otherwise, active
  if (gpio_set_direction_output_and_value(SPI_SS_GPIO_ID, val) != GPIO_RET_SUCCESS) {
    printf("\n\r Failed to set SPI SS (GPIO id-%d) %d", SPI_SS_GPIO_ID, val);
    return WE_FAIL;
  }

  return WE_SUCCESS;
}


static int8_t spi_transfer_data(uint8_t *txData, uint8_t *rxData, uint8_t datalength)
{
  spi_transfer_t transfer_data_tx;
  spi_transfer_t transfer_data_rx;
  int ret_val = 0;

  /* 1. configure spi parameters */
  if(!spi_is_configured) return WE_FAIL;


  memset(&transfer_data_tx, 0, sizeof(spi_transfer_t));
  transfer_data_tx.slaveId = SPIM_SS_1;
  transfer_data_tx.sendData = txData;
  transfer_data_tx.recvData = NULL;
  transfer_data_tx.len = datalength;

  memset(&transfer_data_rx, 0, sizeof(spi_transfer_t));
  transfer_data_rx.slaveId = SPIM_SS_1;
  transfer_data_rx.sendData = NULL;
  transfer_data_rx.recvData = rxData;
  transfer_data_rx.len = datalength;

  set_ss_active(0);

  if ((ret_val = spi_send(SPIM1_BUS, &transfer_data_tx)) <= 0) {
    printf("\n\r Failed to Send data. error:%d!!", ret_val);
    set_ss_active(1);
    return WE_FAIL;
  }

  if ((NULL != rxData) && ((ret_val = spi_receive(SPIM1_BUS, &transfer_data_rx)) <= 0)) {
    printf("\n\r Failed to Receive data. error:%d!!", ret_val);
    set_ss_active(1);
    return WE_FAIL;
  }

  set_ss_active(1);

  return WE_SUCCESS;
}

int8_t SPI_ReadReg(uint8_t RegAdr, int NumByteToRead, uint8_t *Data)
{
    if(NULL == Data)
	{
		return WE_FAIL;
	}

    uint8_t txData[2];
	for(int i = 0; i < NumByteToRead; i++)
	{
		txData[0] = (RegAdr + i) | SPI_READ_MASK;

		if(WE_FAIL == spi_transfer_data(txData, &Data[i], 1))
			return WE_FAIL;
	}

	return WE_SUCCESS;
}


int8_t SPI_WriteReg(uint8_t RegAdr, int NumByteToWrite, uint8_t *Data)
{
	if(NULL == Data)
	{
		return WE_FAIL;
	}

	uint8_t txData[2];
	for(int i = 0; i < NumByteToWrite; i++)
	{
		/* Least significant byte is send first, so fill in tx data "backwards"*/
		txData[0] = Data[i];
		txData[1] = RegAdr + i;

		if(WE_FAIL == spi_transfer_data(txData, NULL, 2))
			return WE_FAIL;
	}

	return WE_SUCCESS;
}

/**
*@brief Print a status string with formatting
*@param   - str: String to print
*         - status: ok or nok
*@retval None
*/
void Debug_out(char *str, bool status)
{
		printf("%s\n", str);
}


/**
 * @brief Sleep for specified time
 * @param time period in ms
 * @retval None
 */
void delay(unsigned int sleepForMs)
{
	struct timespec sleeper, dummy;

	sleeper.tv_sec = (time_t)(sleepForMs / 1000);
	sleeper.tv_nsec = (long)(sleepForMs % 1000) * 1000000;

	//nanosleep(&sleeper, &dummy);
}

/**         EOF         */
