# ALT125x MCU SDK repository

This repository enable you to download the MCU SDK source files and documentation. At this point no commits are allowed.
```
$ git clone --recursive https://github.com/sonydevworld/altair_mcu.git
```
Detailed information on the MCU SDK SW componenet, structure, toolchain and working environment can be found in the [SONY developer portal](https://developer.sony.com/develop/cellular-iot/altair-docs/1250_MCU_Software_Distribution_Manual_en.html#_getting_started)

# New features and changes introduce in MCU SDK V2.2.1
## OS-less Driver 
1. DRV_AUXADC - New function to query modem temperature reading from PMP directly, instead of awaking MAP through AT command.
2. DRV_UART - Add UART Rx break interrupt to detect UART disconnection from external host.
## Applib
1. Support to build image on Windows Command Prompt.
## Example Projects
1. AuxAdc - Add CLI command to demo the new function of DRV_AUXADC.
2. FsioConnector - New example of remotely accessing MAP filesystem.
3. GpsConnector - Add example code for using new C-API for receiving all types of NMEA message.
4. LteConnector - Add example code for using new C-API for registering PDN activation.
## FreeRTOS
1. Apply fix for CVE-2021-31571 vulnerability.
## Middleware altcomlib
1. New FS connector (version 0.2) - New C-APIs on remotely accessing MAP filesystem.
2. New IO connector (version 0.1) - New C-APIs on remotely accessing MAP filesystem.
3. GNSS connector (version 0.2) - New C-API to support raw NMEA type by receiving all types of NMEA message.
4. LTE connector (version 0.10) - New API to register PDN activation callback.
5. Socket connector (version 0.3) - Add RAI support.
6. Serial - Add the registration of UART Rx break interrupt.
          
# Dependencies 
## Required RK and CP versions
To test with the ALT1250, use Modem release RK_03_00_00_00_12251_001 and above ; RK_03_02_00_00_12251_001 and above. 
To test with the ALT1255, use CP_01_00_00_00_12251_001 and above. 
Preveious RK and CP version are supported but new features that were introduce in MCU SDK 2.2.1 will not be available. 

## IO Partition (released together with SDK package)
The corresponding io_par.bin for the ALT1250 MCU Dev Kit and ALT1255 NB Dev Kit are available in the images folders:

./images/io_par_ALT1250.bin

./images/io_par_ALT1255.bin

./images/io_par_ALT1255_EJTAG.bin (for MCU JTAG debug on the ALT1255 NB Dev Kit)

# Limitations and special instructions
On the ALT1255 platform, the following Example projects are NOT supported because the corresponding modem components are disabled:
1. AwsConnector
2. GpsConnector
3. HttpConnector
4. MqttConnector
5. PwrMgmtAwsDemo
6. SmsConnector
