/**
 ***************************************************************************************************
 * This file is part of Sensors SDK:
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

#ifndef _WE_PLATTFORM_H
#define _WE_PLATTFORM_H

 /**         Includes         */

#include <stdint.h>
#include <stdbool.h>

#define WE_SUCCESS   0
#define WE_FAIL     -1
#define SPI_READ_MASK (1 << 7)
//#define USE_SPI


/**         Functions definition         */



/* Initializes i2c periphheral for communicaiton with a sensor. either use I2CInit() or SpiInit(). */
extern int8_t I2CInit(int address);
extern int8_t I2C_SetMode(int mode);
extern int8_t I2C_SetAddress(int address);
extern int8_t SPI_Init();
/* 
 * Initalizes SPI peripheral for communicaiton with a sensor. either use I2CInit() or SpiInit(). 
 * USE_SPI (this file, line 56) must be defined to use the SPI as communication interface. 
 */
//int8_t SpiInit(int channel);

/* Read a register's content */
extern int8_t I2C_ReadReg(uint8_t RegAdr, int NumByteToRead, uint8_t *Data);
extern int8_t SPI_ReadReg(uint8_t RegAdr, int NumByteToRead, uint8_t *Data);

/* write a register's content */
extern int8_t I2C_WriteReg(uint8_t RegAdr, int NumByteToWrite, uint8_t *Data);
extern int8_t SPI_WriteReg(uint8_t RegAdr, int NumByteToWrite, uint8_t *Data);

/* debug output */
extern void Debug_out(char *str, bool status);

/* provides means for a delay in 1 ms resolution. blocking. */
extern void delay(unsigned int sleepForMs);

#endif /* _WE_PLATTFORM_H */
/**         EOF         */
