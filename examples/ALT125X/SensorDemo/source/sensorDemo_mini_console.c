/****************************************************************************
 *
 *  (c) copyright 2019 Altair Semiconductor, Ltd. All rights reserved.
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

/* Standard includes. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
//#include "serial.h"
#include "serial_container.h"
#include "newlibPort.h"
#include "mini_console.h"
#include "platform.h"
#include "WSEN_TIDS.h"
#include "WSEN_ITDS.h"
#include "WSEN_PADS.h"
#include "WSEN_HIDS.h"

//#include "../../../../devices/ALT125X/time/timex.c"

//#include "../Eclipse/We_Sensor/WSEN_HIDS_2523020210001/drivers/WSEN_HIDS.h"
//#include "../Eclipse/We_Sensor/WSEN_HIDS_2523020210001/drivers/WSEN_HIDS.c"

//#include "../Eclipse/We_Sensor/Adrastea/ATCommands/ATMQTT.h"
//#include "../Eclipse/We_Sensor/Adrastea/ATCommands/ATPacketDomain.h"
//#include "../Eclipse/We_Sensor/Adrastea/ATCommands/ATEvent.h"

//#include "../Eclipse/We_Sensor/global/global.h"
//#include "../Eclipse/We_Sensor/Adrastea/ATCommands/ATMQTT.c"
//#include "../Eclipse/We_Sensor/AdrasteaI/Examples/AdrasteaExamples.h"

//#include "../Eclipse/We_Sensor/WSEN_HIDS_2523020210001/AdrasteaI/Examples/AdrasteaExamples.h"
//#include "../Eclipse/We_Sensor/WSEN_HIDS_2523020210001/AdrasteaI/Examples/AdrasteaExamples.c"

//Mqtt
/*  void Adrastea_ATMQTT_EventCallback(char *eventText);

  static ATPacketDomain_Network_Registration_Status_t status = {
  		.state = 0 };
  static ATMQTT_Connection_Result_t conResult = {
  		.resultCode = -1 };
  static ATMQTT_Subscription_Result_t subResult = {
  		.resultCode = -1 };
*/

serial_handle *sHandle_i = NULL;
char testbuf[20];
static volatile uint32_t head = 0, tail = 0;
static int i2c_initializes = 0;

static int pads_inizialized = 0;
static int initialize_pads();

// variables for store sensor value
float pads_temp;



static void prvAtCmdTask(void *pvParameters) {
  sHuart uartInit;
  char c;

  serial_load_defconfig(ACTIVE_UARTI0, &uartInit);
  sHandle_i = serial_open(&uartInit);
  configASSERT(sHandle_i);

  while (1) {
    serial_read(sHandle_i, &c, 1);
    //printf("in line62");
    console_write(&c, 1);
  }
}

static void prvSPITransferTask(void *pvParameters){
  if (WE_FAIL == SPI_Init())
  {
    printf("Could not initalize spi \n\r");
    return;
  }
  printf("SPI initialized \n\r");

  printf("Start SPI transfer\r\n");
  uint8_t rxData;
  while (1){
	  // Just do anything with SPI. Reading a register of a sensor sends a message to the sensor and waits for a read.
	  ITDS_getDeviceID(&rxData);
	  vTaskDelay(5);
  }
}

int do_test(char *s){

  static xTaskHandle SPITaskHandle = NULL;
  int argc;
  char command[20] = {0};
  uint32_t testRuns = 0;
  argc = sscanf(s, "%s", command) + 1;

  if (argc <= 1) {
    printf("Invalid parameter \n\r");
	return -1;
  } else {
	testRuns = strtol(command,NULL,10);
  }

  if(!i2c_initializes)
    {
	  if((WE_SUCCESS == I2CInit(TIDS_ADDRESS_I2C_1)) && (WE_SUCCESS == I2CInit(ITDS_ADDRESS_I2C_1)))
	  {
		  i2c_initializes = 1;
	  }
	  else
	  {
		  printf("Could not initialized I2C\n\r");
		  return -1;
	  }
  }

  // Create a Task do perform a spi transfer in parallel
  xTaskCreate(prvSPITransferTask, /* The function that implements the task. */
              "SPI transfer", /* The text name assigned to the task - for debug only as it is not used by
                            the kernel. */
              configMINIMAL_STACK_SIZE, /* The size of the stack to allocate to the task. */
              NULL,                     /* The parameter passed to the task */
              configMAX_PRIORITIES - 1, /* The priority assigned to the task - minimal priority. */
              &SPITaskHandle);

  vTaskDelay(500);
  uint8_t rxData;
  for(uint32_t i = 0; i < testRuns; i++)
  {
	  TIDS_getDeviceID(&rxData);
  }

  for(uint32_t i = 0; i < testRuns; i++)
  {
	  ITDS_getDeviceID(&rxData);
  }

  //end SPI task and therefore spi transfer
  vTaskDelete(SPITaskHandle);
  printf("Test done.\r\n");

  return 0;
}

int do_map2(char *s) {
  char c;
  char p1[]="at\n";
    char p2[]="AT+CPIN?\n";
    char str_pads_temp[50];
    snprintf(str_pads_temp, sizeof(str_pads_temp), "%f", pads_temp);

  static xTaskHandle AtCmdTaskHandle = NULL;
  xTaskCreate(prvAtCmdTask, /* The function that implements the task. */
              "Console", /* The text name assigned to the task - for debug only as it is not used by
                            the kernel. */
              configMINIMAL_STACK_SIZE, /* The size of the stack to allocate to the task. */
              NULL,                     /* The parameter passed to the task */
              configMAX_PRIORITIES - 1, /* The priority assigned to the task - minimal priority. */
              &AtCmdTaskHandle);

  printf("Open MAP 2 CLI (Press Ctrl+D to exit)\r\n");
  do {
    console_read(&c, 1);

    if (c != 4) {
      //serial_write(sHandle_i, &c, 1);

      for(int i=0;i<strlen(p1);i++)
         {
           serial_write(sHandle_i, &p1[i], 1);
          }
            //WE_Delay(1000);

       serial_write(sHandle_i, &c, 1);
       WE_Delay(200);
       for(int i=0;i<strlen(p2);i++)
        {
          serial_write(sHandle_i, &p2[i], 1);
         }
       serial_write(sHandle_i, &c, 1);
       WE_Delay(200);
    }
  } while (c != 4);

  serial_close(sHandle_i);
  printf("MAP CLI Closed.\r\n");

    sHandle_i = NULL;
    vTaskDelete(AtCmdTaskHandle);

  return 0;
}


int do_map(char *s) {
  char c[50];
  char p1[]="at\n";
  char p2[]="AT+CPIN?\n";
  char str_pads_temp[50];
  snprintf(str_pads_temp, sizeof(str_pads_temp), "%f", pads_temp);

  static xTaskHandle AtCmdTaskHandle = NULL;
  xTaskCreate(prvAtCmdTask, /* The function that implements the task. */
              "Console", /* The text name assigned to the task - for debug only as it is not used by
                            the kernel. */
              configMINIMAL_STACK_SIZE, /* The size of the stack to allocate to the task. */
              NULL,                     /* The parameter passed to the task */
              configMAX_PRIORITIES - 1, /* The priority assigned to the task - minimal priority. */
              &AtCmdTaskHandle);

  printf("Open MAP CLI (Press Ctrl+D to exit)\r\n");
  do {

    if (c != 4) {
      // If this next line is commented out, no OK response is received after sending AT
    	serial_write(sHandle_i, &c, 1);
      for(int i=0;i<strlen(p1);i++)
        {
      	  serial_write(sHandle_i, &p1[i], 1);
        }
      //WE_Delay(1000);

      serial_write(sHandle_i, &c, 1);
            for(int i=0;i<strlen(p2);i++)
              {
            	  serial_write(sHandle_i, &p2[i], 1);

              }

            /*serial_write(sHandle_i, &str_pads_temp, 1);
                        for(int i=0;i<strlen(str_pads_temp);i++)
                          {
                        	  serial_write(sHandle_i, &str_pads_temp[i], 1);

                          }*/

    }

    console_read(&c, 1);
    //serial_read(sHandle_i,&c,1);



  } while (c != 4);

  //strcpy(c,"at");

  serial_write(sHandle_i, &c, 1);
  for(int i=0;i<3;i++)
  {
	  // serial_write(sHandle_i, &p[i], 1);
	  printf("exit\n");
  }


  WE_Delay(1000);

  /*for(int i=0;i<1;i++)
  {

   for(int i=0;i<25;i++)
   {
   printf("doing temp read from TIDS iteration %d ", i);
   do_tids("temp");
   }
   printf(do_tids("id"));

   printf("\n");
   printf("\n");


   for(int i=0;i<25;i++)
    {
 	  printf("doing temp read from ITDS iteration %d ", i);
 	  do_itds("temp");
 	  printf("doing accel read from ITDS iteration %d ", i);
 	  do_itds("accel");
    }
   printf( do_itds("id"));

    printf("\n");
    printf("\n");


    for(int i=0;i<25;i++)
     {
  	  printf("doing temp read from PADS iteration %d ", i);
  	  do_pads("temp");
     }
    printf(do_pads("id"));
    printf("\n");
    printf("\n");
   }


  /*if (!Adrastea_Init(115200, WE_FlowControl_NoFlowControl, WE_Parity_None, &Adrastea_ATMQTT_EventCallback, NULL))
  	{
  		return;
  	}*/

  	printf("*** Start of Adrastea ATMQTT example ***\r\n");

  	WE_Delay(1000);


  printf("MAP CLI Closed.\r\n");
  serial_close(sHandle_i);
  sHandle_i = NULL;
  vTaskDelete(AtCmdTaskHandle);

  return 0;
}

void printTidsUsage()
{
	printf("Usage:\n\r");
	printf("tids id - get the i2c device address of the tids sensor\n\r");
	printf("tids temp - get the temperature in celsius\n\r");
}

void printItdsUsage()
{
	printf("Usage:\n\r");
	printf("itds id - get the device id of the sensor\n\r");
	printf("itds temp - get the temperature in celsius\n\r");
}

void printPadsUsage()
{
	printf("Usage:\n\r");
	printf("pads id - get the device id of the sensor\n\r");
}

void printHidsUsage()
{
	printf("Usage:\n\r");
	printf("hids id - get the device id of the sensor\n\r");
}

int do_i2c_speed(char *s)
{
	int argc;
	char command[20] = {0};

	argc = sscanf(s, "%s", command) + 1;

	if (argc <= 1) {
		printf("Invalid parameter \n\r");
		return -1;
	} else {
		int mode = strtol(command,NULL,10);
		if (0 != I2C_SetMode(mode))
		{
			printf("Could not set i2c speed to %i", mode);
			return -1;
		}

		printf("speed set to %i", mode);
	}

	return 0;
}

int do_tids(char *s)
{
	int argc;
	int ret_val = 0;
	char command[20] = {0};

	argc = sscanf(s, "%s", command) + 1;

  if (argc <= 1) {
    printTidsUsage();
    ret_val = -1;
  } else {
	  if(!i2c_initializes)
	  {
		  if(WE_SUCCESS == I2CInit(TIDS_ADDRESS_I2C_1))
		  {
			  i2c_initializes = 1;
		  }
		  else
		  {
			  printf("Could not initialized I2C\n\r");
			  return -1;
		  }

	  }

      if (strcmp("id", command) == 0) {

    	 uint8_t id = 0;
    	 I2C_SetAddress(TIDS_ADDRESS_I2C_1);
    	 if(WE_SUCCESS == TIDS_getDeviceID(&id))
    	 {
        	  printf("ID: %02x\n\r", id);
        	  //printf("Adrastea exampl write: ");
        	  //char message[] = "Hello, world!";
        	  //bool isSuccess = true;
        	  //AdrasteaExamplesPrint(message, isSuccess);
        	  Adrasteatest1();
        	  msleep(1000);

        	  Adrasteatest1();

        	  //Adrasteatest1();
        	  //ATDeviceExample();
    	 }
    	 else
    	 {
    		 printf("Could not read out device id \n\r");
    		 return -1;
    	 }

      }
      else if(strcmp("temp", command) == 0)
      {
    	  I2C_SetAddress(TIDS_ADDRESS_I2C_1);
    	  if(WE_FAIL == TIDS_setSingleConvMode(TIDS_enable))
    	  {
    		  printf("Could not set single conv\n\r");
    		  return -1;
    	  }

    	float temp = 0;
    	if(WE_SUCCESS == TIDS_getTemperature(&temp))
    	{
    		printf("Temperature (C): %f\n\r", temp);
    	}
    	else
    	{
    		printf("Could not read out temperature \n\r");
    		return -1;
    	}
      }
      else
      {
    	  printTidsUsage();
    	  ret_val = -1;
      }
  }

	return ret_val;
}

int do_itds(char *s)
{
	int argc;
		int ret_val = 0;
		char command[20] = {0};

		argc = sscanf(s, "%s", command) + 1;

	  if (argc <= 1) {
	    printItdsUsage();
	    ret_val = -1;
	  } else {
		  if(!i2c_initializes)
		  {
			  if(WE_SUCCESS == I2CInit(ITDS_ADDRESS_I2C_1))
			  {
				  i2c_initializes = 1;
			  }


			  else
			  {
				  printf("Could not initialized I2C\n\r");
				  return -1;
			  }

		  }

	      if (strcmp("id", command) == 0) {
	    	  I2C_SetAddress(ITDS_ADDRESS_I2C_1);
	    	 uint8_t id = 0;
	    	 if(WE_SUCCESS == ITDS_getDeviceID(&id))
	    	 {
	    		 //printf("Print ID");
	        	  printf("ID: %02x\n\r", id);
	    	 }
	    	 else
	    	 {
	    		 printf("Could not read out device id \n\r");
	    		 return -1;
	    	 }

	      }
	     else if(strcmp("temp", command) == 0)
	      {
	    	float temp12bit = 0;
	    	I2C_SetAddress(ITDS_ADDRESS_I2C_1);

	    	if(WE_SUCCESS == ITDS_getTemperature12bit(&temp12bit))
	    	 {
	    	    printf("Temperature 12 bit (C): %f\n\r", temp12bit);
	    	  }

	    	else
	    	{
	    		printf("Could not read out temperature \n\r");
	    		return -1;
	    	}
	      }

	      else if(strcmp("accel", command) == 0)
	      {
	    	  I2C_SetAddress(ITDS_ADDRESS_I2C_1);
			ITDS_state_t DRDY = ITDS_disable;
			int16_t XRawAcc = 0, YRawAcc = 0, ZRawAcc = 0;

			// Sampling rate of 200 Hz
			ITDS_setOutputDataRate(odr6);
			// Enable normal mode
			ITDS_setOperatingMode(normalOrLowPower);
			ITDS_setpowerMode(normalMode);
			// Enable block data update
			ITDS_setBlockDataUpdate(ITDS_enable);
			// Enable address auto increment
			ITDS_setAutoIncrement(ITDS_enable);
			// Full scale +-16g
			ITDS_setFullScale(sixteenG);
			// Filter bandwidth = ODR/2
			ITDS_setFilteringCutoff(outputDataRate_2);

			uint8_t retries = 0;
			do
			{
				ITDS_getdataReadyState(&DRDY);
				retries++;
			} while ((DRDY == ITDS_disable) && (retries < 10));

			if(retries < 10)
			{
				ITDS_getRawAccelerationX(&XRawAcc);
				XRawAcc = XRawAcc >> 2; // shifted by 2 as 14bit resolution is used in normal mode
				float XAcceleration = (float) (XRawAcc);
				XAcceleration = XAcceleration / 1000; // mg to g
				XAcceleration = XAcceleration * 1.952; // Multiply with sensitivity 1.952 in normal mode, 14bit, and full scale +-16g
				printf("Acceleration X-axis %f g \r\n", XAcceleration);

				ITDS_getRawAccelerationY(&YRawAcc);
				YRawAcc = YRawAcc >> 2;
				float YAcceleration = (float) (YRawAcc);
				YAcceleration = YAcceleration / 1000;
				YAcceleration = (YAcceleration*1.952);
				printf("Acceleration Y-axis %f g \r\n", YAcceleration);

				ITDS_getRawAccelerationZ(&ZRawAcc);
				ZRawAcc = ZRawAcc >> 2;
				float ZAcceleration = (float) (ZRawAcc);
				ZAcceleration = ZAcceleration / 1000;
				ZAcceleration = ZAcceleration * 1.952;
				printf("Acceleration Z-axis %f g \r\n", ZAcceleration);
			}
			else
			{
				printf("Sensor not ready");
			}
	      }

	      else
	      {
	    	  printItdsUsage();
	    	  ret_val = -1;
	      }
	  }

		return ret_val;

}

int do_hids(char *s)
{
	int argc;
	int status;
	//The Output Data Rate in Hz
	int ODR = 0;
	int ret_val = 0;
	char command[20] = {0};

		argc = sscanf(s, "%s", command) + 1;

	  if (argc <= 1) {
	    printHidsUsage();
	    ret_val = -1;
	  } else {
		  if(!i2c_initializes)
		  {


			  if(WE_SUCCESS == I2CInit(HIDS_ADDRESS_I2C_0))
			  {
				  i2c_initializes = 1;


				  if (WE_FAIL == HIDS_setOdr(ODR))
				  	{
					  printf("Could not initialized output data rate\n\r");
				  		return WE_FAIL;
				  	}

				  if (WE_FAIL == HIDS_setPowerMode(activeMode))
				  	{
					  printf("Could not initialized power mode\n\r");
				  		return WE_FAIL;

				  	}

			  }
			  else
			  {
				  printf("Could not initialized I2C\n\r");
				  return -1;
			  }

		  }

	      if (strcmp("id", command) == 0) {



	    	 uint8_t id = 0;
	    	 I2C_SetAddress(HIDS_ADDRESS_I2C_0);
	    	 if(WE_SUCCESS == HIDS_getDeviceID(&id))
	    	 {
	        	  printf("ID: %02x\n\r", id);
	    	 }
	    	 else
	    	 {
	    		 printf("Could not read out device id \n\r");
	    		 return -1;
	    	 }

	      }

	      else if (strcmp("humid", command) == 0) {
	    	  I2C_SetAddress(HIDS_ADDRESS_I2C_0);
	    	  HIDS_state_t humidity_drdy, temp_drdy;
	    	  //humidity_drdy = HIDS_enable;


	    	    if (WE_FAIL == HIDS_enOneShot(HIDS_enable))
	    	    {
	    	    	printf("Could not set to one shot \r\n");
	    	    }


	    	    // HIDS_getHumStatus(&humStatus);
	    	    float humid = 0;

	    	    if(WE_SUCCESS == HIDS_getHumidity(&humid))
	    	    {
	    	    	printf("humidity : %f\n\r", humid);
	    	    }
	    	    else
	    	    {
	    	    	printf("Could not read out humid \n\r");
	    	    	return -1;
	    	    }

	      }

	      else if (strcmp("temp", command) == 0) {
	 	    	  I2C_SetAddress(HIDS_ADDRESS_I2C_0);
	 	    	  //HIDS_state_t humStatus = HIDS_disable, tempStatus = HIDS_disable;
	 	    	 HIDS_state_t tempStatus = HIDS_enable;

	 	    	    if (WE_FAIL == HIDS_enOneShot(HIDS_enable))
	 	    	    {
	 	    	    	printf("Could not set to one shot \r\n");
	 	    	    }

	 	    	    // Get status of data

			HIDS_getTempStatus(&tempStatus);

	 	    	    float temp = 0;
         	    if(WE_SUCCESS == HIDS_getTemperature(&temp))
	 	    	    {
	 	    	    	printf("temp (C) : %f\n\r", temp);
	 	    	    }
	 	    	    else
	 	    	    {
	 	    	    	printf("Could not read out temp \n\r");
	 	    	    	return -1;
	 	    	    }
         	   SW_RESET();

	 	 	      }


	      else
	      {
	    	  printHidsUsage();
	    	  ret_val = -1;
	      }
	  }

		return ret_val;
}


int do_pads(char *s)
{
	int argc;
	int ret_val = 0;
	char command[20] = {0};

	argc = sscanf(s, "%s", command) + 1;

  if (argc <= 1) {
	printPadsUsage();
    ret_val = -1;
  } else {

	  if (WE_FAIL == I2CInit(PADS_ADDRESS_I2C_1))
	  {
		 printf("Could not initalize i2c \n\r");
		  return -1;
	  }
	  // printf("I2C initialized \n\r");

      if (strcmp("id", command) == 0) {
    	 uint8_t id = 0;
    	 I2C_SetAddress(PADS_ADDRESS_I2C_1);
    	 if(WE_SUCCESS == PADS_getDeviceID(&id))
    	 {
    		 printf("ID: %02x\n\r", id);
    	 }
    	 else
    	 {
    		 printf("Could not read out device id \n\r");
    		 return -1;
    	 }
      }
      else if (strcmp("temp", command) == 0)
      {
    	 if(WE_SUCCESS != initialize_pads())
    	 {
    	  printf("Could not initalize pads sensor\n\r");
    	  return -1;
    	 }
    	 I2C_SetAddress(PADS_ADDRESS_I2C_1);
	     PADS_setSingleConvMode(PADS_enable);
    	 vTaskDelay(15);

    	float temp = 0;
    	if(WE_SUCCESS == PADS_getTemperature(&temp))
		{
			printf("Temperature (C): %f\n\r", temp);
			pads_temp = temp;
		}
		else
		{
			printf("Could not read out temperature \n\r");
			return -1;
		}

      }
      else if (strcmp("pressure", command) == 0)
      {
    	  I2C_SetAddress(PADS_ADDRESS_I2C_1);
    	  if(WE_SUCCESS != initialize_pads())
    	 {
    	  printf("Could not initalize pads sensor\n\r");
    	  return -1;
    	 }
    	 // printf("initialize sucess \n\r");
	     PADS_setSingleConvMode(PADS_enable);
    	 vTaskDelay(15);

    	float pressure = 0;
    	if(WE_SUCCESS == PADS_getPressure(&pressure))
		{
			printf("Pressure (kPa): %f\n\r", pressure);
		}
		else
		{
			printf("Could not read out pressure\n\r");
			return -1;
		}

      }
      else
      {
    	printPadsUsage();
    	return -1;
      }
  }

  return ret_val;
}

static int initialize_pads()
{
	if (1 == pads_inizialized)
	{
		return WE_SUCCESS;
	}

	int retVal = WE_SUCCESS;

	/*set ODR to Power down*/
	retVal &= PADS_setOutputDataRate(pads_outputDataRatePowerDown);

	/*set low pass filter to BW=ODR/20*/
	if(retVal) retVal &= PADS_setLowPassFilterConf(pads_lPfilterBW2);

	/*enable low pass filter */
	if (retVal) retVal &= PADS_setLowPassFilter(PADS_enable);

	/*enable Block Data Update*/
	if (retVal) retVal &= PADS_setBlockDataUpdate(PADS_enable);

	/*select Power Mode [0:low Current Mode; 1:low noise Mode]*/
	if (retVal) retVal &= PADS_setPowerMode(pads_lowNoise);

	/*enable the Auto Increment */
	if (retVal) retVal &= PADS_setAutoIncrement(PADS_enable);


	pads_inizialized = retVal;

	return retVal;
}


// MQTT
/*void Adrastea_ATMQTT_EventCallback(char *eventText)
{
	ATEvent_t event;
	ATEvent_ParseEventType(&eventText, &event);

	switch (event)
	{
	case ATEvent_MQTT_Connection_Confirmation:
	{
		ATMQTT_ParseConnectionConfirmationEvent(eventText, &conResult);
		break;
	}
	case ATEvent_MQTT_Subscription_Confirmation:
	{
		ATMQTT_ParseSubscriptionConfirmationEvent(eventText, &subResult);
		break;
	}
	case ATEvent_MQTT_Publication_Received:
	{
		ATMQTT_Publication_Received_Result_t result;
		char payload[128];
		result.payload = payload;
		result.payloadMaxBufferSize = 128;
		if (!ATMQTT_ParsePublicationReceivedEvent(eventText, &result))
		{
			return;
		}
		printf("Connection ID: %d, Message ID: %d, Topic Name: %s, Payload Size: %d, Payload: %s\r\n", result.connID, result.msgID, result.topicName, result.payloadSize, result.payload);
		break;
	}
	case ATEvent_PacketDomain_Network_Registration_Status:
	{
		ATPacketDomain_ParseNetworkRegistrationStatusEvent(eventText, &status);
		break;
	}
	default:
		break;
	}
}*/
