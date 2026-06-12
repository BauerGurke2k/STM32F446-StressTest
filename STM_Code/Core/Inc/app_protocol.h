/*
 * app_protocol.h
 *
 *  Created on: 12.06.2026
 *      Author: Gemini
 */

#ifndef INC_APP_PROTOCOL_H_
#define INC_APP_PROTOCOL_H_

#include "stm32f4xx_hal.h"

// Configuration for the protocol timings
#define PROTOCOL_USB_STATUS_INTERVAL_MS 100 // Interval for sending USB status packets
#define PROTOCOL_CAN_HEARTBEAT_INTERVAL_MS 500 // Interval for sending CAN heartbeat

// CAN IDs
#define CAN_ID_HEARTBEAT    0x100
#define CAN_ID_TESTCOUNTER  0x101
#define CAN_ID_ERRORCOUNTER 0x102
#define CAN_ID_STATUS       0x103
#define CAN_ID_REQUEST_PI   0x200
#define CAN_ID_RESPONSE_MCU 0x201

void protocol_init(void);
void protocol_run(void);
void protocol_handle_can_rx(CAN_RxHeaderTypeDef *pHeader, uint8_t aData[]);

#endif /* INC_APP_PROTOCOL_H_ */
