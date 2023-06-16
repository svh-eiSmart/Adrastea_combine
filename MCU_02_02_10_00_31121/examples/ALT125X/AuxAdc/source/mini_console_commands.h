/*  ---------------------------------------------------------------------------

    (c) copyright 2021 Altair Semiconductor, Ltd. All rights reserved.

    This software, in source or object form (the "Software"), is the
    property of Altair Semiconductor Ltd. (the "Company") and/or its
    licensors, which have all right, title and interest therein, You
    may use the Software only in  accordance with the terms of written
    license agreement between you and the Company (the "License").
    Except as expressly stated in the License, the Company grants no
    licenses by implication, estoppel, or otherwise. If you are not
    aware of or do not agree to the License terms, you may not use,
    copy or modify the Software. You may use the source code of the
    Software only for your internal purposes and may not distribute the
    source code of the Software, any part thereof, or any derivative work
    thereof, to any third party, except pursuant to the Company's prior
    written consent.
    The Software is the confidential information of the Company.

   ------------------------------------------------------------------------- */

/* Declare extra CLI commands in this file */

#undef MCU_PROJECT_NAME
#define MCU_PROJECT_NAME "auxadc"
#if defined(ALT1250)
DECLARE_COMMAND("auxadc_get", do_auxadc_get,
                "auxadc_get <channel> - get ADC value from corresponding channel. Valid channel "
                "number is from 0 to 3")
DECLARE_COMMAND("auxadc_int_reg", do_auxadc_int_reg,
                "auxadc_int_reg <channel> - Register interrupt handler for an ADC channel. Valid "
                "channel number is from 0 to 3")
DECLARE_COMMAND("auxadc_int_dereg", do_auxadc_int_dereg,
                "auxadc_int_dereg <channel> - Deregister interrupt handler for an ADC channel. "
                "Valid channel number is from 0 to 3")

#elif defined(ALT1255)
DECLARE_COMMAND("auxadc_get", do_auxadc_get,
                "auxadc_get <channel> - get ADC value from corresponding channel. Valid channel "
                "number is 2")
DECLARE_COMMAND("auxadc_int_reg", do_auxadc_int_reg,
                "auxadc_int_reg <channel> - Register interrupt handler for an ADC channel. Valid "
                "channel number is 2")
DECLARE_COMMAND("auxadc_int_dereg", do_auxadc_int_dereg,
                "auxadc_int_dereg <channel> - Deregister interrupt handler for an ADC channel. "
                "Valid channel number is 2")

#else
#error "Wrong platform configuration"
#endif

DECLARE_COMMAND("auxadc_scale", do_auxadc_set_scale, "auxadc_scale <scale> - Set ADC scale factor")
DECLARE_COMMAND("auxadc_vbat_get", do_auxadc_vbat_get,
                "auxadc_vbat_get - Get battery sensor reading in mV")
DECLARE_COMMAND("auxadc_temp_get", do_auxadc_temperature_get,
                "auxadc_temp_get - Get Temperature")