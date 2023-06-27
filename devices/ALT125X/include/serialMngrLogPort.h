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

#ifndef CORE_INCLUDE_MANAGER_SERIAL_SERIALMNGRLOGPORT_H_
#define CORE_INCLUDE_MANAGER_SERIAL_SERIALMNGRLOGPORT_H_

#include <stdint.h>
//#include "serial_container.h"

// typedef void (*serialRxProcessFp_t)(const char charRecv,void *recvCookie);
// //typedef int (*serialMngrTxcharFp_t)(void *serialHandle,const char *txCharBuf,unsigned short
// txCharBufLen); typedef int (*serialMngrTxcharFp_t)(serial_handle * handle, char *buf, uint32_t
// len );

/* read only enum do not modify */
typedef enum logical_hw_representation_id {
  // the representation definition must not be changed
  LS_REPRESENTATION_ID_UART_A = 0, /* logical */
  LS_REPRESENTATION_ID_UART_B,
  LS_REPRESENTATION_ID_UART_0,
  LS_REPRESENTATION_ID_UART_1,
  LS_REPRESENTATION_ID_UART_2,
  LS_REPRESENTATION_LAST,
} logical_hw_representation_id_t;

// the logical ports may be defined by the application layer
typedef enum {
  LOGICAL_PORT_TO_AT_CLIENT_APP_PORT0_DEF = LS_REPRESENTATION_LAST,
  LOGICAL_PORT_TO_PPP_APP_SWITCH_PORT1,
  LOGICAL_PORT_CLIENT_TO_DATA_APP_PORT0_DEF,
  LOGICAL_PORT_CLIENT_TO_DATA_APP_PORT1,
  LOGICAL_PORT_TO_CLI_APP_SWITCH_PORT0_DEF,
  LOGICAL_PORT_TO_AT_CLIENT1_APP_PORT1,
  LOGICAL_PORT_PROPR1_SWITCH_PORT0_DEF,
  LOGICAL_PORT_PROPR1_SWITCH_PORT1,
  LOGICAL_PORT_AUX_SWITCH_PORT1,
  LOGICAL_PORT_CASC_SWITCH1_SWITCH1_SWITCH2,

  LOGICAL_SERIAL_ID_MAX,

} logical_serial_id_t;

typedef enum { SERIAL_TYPE_UART_ID, SERIAL_TYPE_USB_ID, SERIAL_TYPE_MAX_ID } serial_type_id_t;

enum hwSer_tx_mode { NON_BLOCKING_MODE = 0, BLOCKING_MODE = 1, BLOCKING_ON_TX_FIFO_MODE = 2 };
/*Would need to be omitted and replace by dynamic number */
enum hwSer_channelNumber {
  UART0 = 0,
#ifdef ALT1250
  UART1,
#endif
  UARTI0,
  UARTI1,
#ifdef ALT1250
  UARTI2,
#endif
  MAX_UART_NUM
};

typedef enum {
  IOCTAL_SERIAL_SW_RX_FIFO = 0,
  IOCTAL_SERIAL_SW_TX_FIFO,
  IOCTAL_SERIAL_RX_CB_FUNC,
  IOCTAL_SERIAL_TX_FUNC_PTR,
  IOCTAL_SERIAL_UART_BAUD_RATE,
  IOCTAL_SERIAL_HW_FLOW_CNTRL,
  IOCTAL_SERIAL_HW_FLOW_QUERY,
  IOCTAL_SERIAL_TX_MODE_CNTRL,
  IOCTAL_SERIAL_MAX
} serial_ioctl_id_t;

enum hwSer_hw_flow_mode {
  SERIAL_HW_FLOW_MODE_RTS_CTS_OFF = 0,  // use the at cmd at&k values (at&k0 at&k3)
  SERIAL_HW_FLOW_MODE_RTS_CTS_ON = 3,

};

typedef struct serial_appp_mngr_msg {
  int app_instance_num;
  logical_serial_id_t logical_serial_id;
} serial_appp_mngr_msg_t;

/*
 * serial port # of uart MAC is familair with
 */
typedef enum serialPortNum {
  SERIAL_PORT_NOT_DEFINED = (uint32_t)'N',
  SERIAL_PORT_A = (uint32_t)'A',
  SERIAL_PORT_C = (uint32_t)'C',
  SERIAL_PORT_B = (uint32_t)'B'
} serialPortNum_e;

#endif /* CORE_INCLUDE_MANAGER_SERIAL_SERIALMNGRLOGPORT_H_ */
