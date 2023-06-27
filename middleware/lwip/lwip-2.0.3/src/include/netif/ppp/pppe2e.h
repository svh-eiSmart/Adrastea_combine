/*  ---------------------------------------------------------------------------

	(c) copyright 2017 Altair Semiconductor, Ltd. All rights reserved.

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

#ifndef CORE_CORE_INC_PPPE2E_INC_H_
#define CORE_CORE_INC_PPPE2E_INC_H_
//#include "serialMngrLogPort.h"
typedef enum {
	LOCAL_PPP_SWITCH_SESSION=1,
	PPP_SESSIONID_SWITCH_SESSION=2,
	PPP_AT_SWITCH_SESSION=3,

	MAX_SWITCH_SESSION=4
}PPP_SES_TYPE;



typedef int (*ppp_end2end_cb_fn)(void);
//ppp_end2end_cb_fn serialSwitchSetE2efunc(void);
//ppp_end2end_cb_fn serialSwitchSetE2eTimer(void);
//int ppp_end2end_set_state(PPP_SES_TYPE type,int sessionId,int dactivate_lte);
//void init_pppEnd2EndApp(logical_serial_id_t lgPort);
#endif /* CORE_CORE_INC_PPPE2E_INC_H_ */
