/*
 * app_protocol.c
 *
 *  Created on: 12.06.2026
 *      Author: Gemini
 */

#include "app_protocol.h"
#include "app_stats.h"
#include "usbd_cdc_if.h" // For CDC_Transmit_FS
#include "can.h"         // For CAN handles and functions
#include <stdio.h>
#include <string.h>

// --- Private function prototypes ---
static void protocol_send_usb_status(void);
static void protocol_send_can_heartbeat(void);
static void protocol_send_can_testcounter(void);
static void protocol_send_can_errorcounter(void);
static void protocol_send_can_status(void);
static uint8_t calculate_checksum(const uint8_t* data, size_t len);

// --- Private variables ---
static uint32_t usb_seq_counter = 0;
static uint32_t can_seq_counter = 0;


void protocol_init(void)
{
    // Nothing to do here for now
}

void protocol_run(void)
{
    static uint32_t last_usb_status_ms = 0;
    static uint32_t last_can_heartbeat_ms = 0;
    app_stats_t* stats = stats_get();

    // Send USB status periodically
    if (stats->uptime_ms - last_usb_status_ms >= PROTOCOL_USB_STATUS_INTERVAL_MS)
    {
        last_usb_status_ms = stats->uptime_ms;
        protocol_send_usb_status();
    }

    // Send CAN messages periodically
    if (stats->uptime_ms - last_can_heartbeat_ms >= PROTOCOL_CAN_HEARTBEAT_INTERVAL_MS)
    {
        last_can_heartbeat_ms = stats->uptime_ms;
        protocol_send_can_heartbeat();
        // Stagger other CAN messages
        protocol_send_can_testcounter();
        if(stats->uptime_ms % 2000 < PROTOCOL_CAN_HEARTBEAT_INTERVAL_MS) // Less frequently
        {
           protocol_send_can_errorcounter();
           protocol_send_can_status();
        }
    }
}

void protocol_handle_can_rx(CAN_RxHeaderTypeDef *pHeader, uint8_t aData[])
{
    app_stats_t* stats = stats_get();
    stats->can_rx_packets++;

    if (pHeader->StdId == CAN_ID_REQUEST_PI)
    {
        // Respond with the same sequence number
        CAN_TxHeaderTypeDef   txHeader;
        uint8_t               txData[1];
        uint32_t              txMailbox;

        txHeader.StdId = CAN_ID_RESPONSE_MCU;
        txHeader.ExtId = 0;
        txHeader.RTR = CAN_RTR_DATA;
        txHeader.IDE = CAN_ID_STD;
        txHeader.DLC = 1;
        txData[0] = aData[0]; // Echo sequence number

        if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox) == HAL_OK)
        {
            stats->can_tx_packets++;
        }
        else
        {
            stats->can_tx_errors++;
        }
    }
}


static void protocol_send_usb_status(void)
{
    app_stats_t* stats = stats_get();
    char buffer[128];

    // Format: USBSTAT,seq,uptime_ms,cpu_iter,can_tx,can_rx,can_err,i2c_err,fram_crc_err,usb_busy,crc
    int len = snprintf(buffer, sizeof(buffer) - 5,
        "USBSTAT,%lu,%lu,%llu,%lu,%lu,%lu,%lu,%lu,%lu",
        usb_seq_counter,
        stats->uptime_ms,
        stats->cpu_stress_iterations,
        stats->can_tx_packets,
        stats->can_rx_packets,
        (stats->can_tx_errors + stats->can_busoff_count),
        stats->i2c_errors,
        stats->fram_crc_errors,
        stats->usb_tx_busy
    );

    uint8_t checksum = calculate_checksum((uint8_t*)buffer, len);
    len += snprintf(buffer + len, sizeof(buffer) - len, ",%u\n", checksum);


    if (CDC_Transmit_FS((uint8_t*)buffer, len) == USBD_OK)
    {
        stats->usb_tx_packets++;
        usb_seq_counter++;
    }
    else
    {
        stats->usb_tx_busy++;
    }
}

static void protocol_send_can_heartbeat(void)
{
    app_stats_t* stats = stats_get();
    CAN_TxHeaderTypeDef   txHeader;
    uint8_t               txData[4];
    uint32_t              txMailbox;

    txHeader.StdId = CAN_ID_HEARTBEAT;
    txHeader.ExtId = 0;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.IDE = CAN_ID_STD;
    txHeader.DLC = 4;
    memcpy(txData, &stats->uptime_s, 4);

    if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox) == HAL_OK)
    {
        stats->can_tx_packets++;
    }
    else
    {
        stats->can_tx_errors++;
    }
}

static void protocol_send_can_testcounter(void)
{
    app_stats_t* stats = stats_get();
    CAN_TxHeaderTypeDef   txHeader;
    uint8_t               txData[4];
    uint32_t              txMailbox;

    txHeader.StdId = CAN_ID_TESTCOUNTER;
    txHeader.ExtId = 0;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.IDE = CAN_ID_STD;
    txHeader.DLC = 4;
    memcpy(txData, &can_seq_counter, 4);
    can_seq_counter++;

    if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox) == HAL_OK)
    {
        stats->can_tx_packets++;
    }
    else
    {
        stats->can_tx_errors++;
    }
}

static void protocol_send_can_errorcounter(void)
{
    app_stats_t* stats = stats_get();
    CAN_TxHeaderTypeDef   txHeader;
    uint8_t               txData[8];
    uint32_t              txMailbox;

    txHeader.StdId = CAN_ID_ERRORCOUNTER;
    txHeader.ExtId = 0;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.IDE = CAN_ID_STD;
    txHeader.DLC = 8;
    txData[0] = (uint8_t)stats->can_tx_errors;
    txData[1] = (uint8_t)stats->can_rx_seq_errors;
    txData[2] = (uint8_t)stats->can_busoff_count;
    txData[3] = (uint8_t)stats->can_error_warning_count;
    txData[4] = (uint8_t)stats->can_error_passive_count;
    txData[5] = 0;
    txData[6] = 0;
    txData[7] = 0;


    if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox) == HAL_OK)
    {
        stats->can_tx_packets++;
    }
    else
    {
        stats->can_tx_errors++;
    }
}

static void protocol_send_can_status(void)
{
    app_stats_t* stats = stats_get();
    CAN_TxHeaderTypeDef   txHeader;
    uint8_t               txData[8];
    uint32_t              txMailbox;

    txHeader.StdId = CAN_ID_STATUS;
    txHeader.ExtId = 0;
    txHeader.RTR = CAN_RTR_DATA;
    txHeader.IDE = CAN_ID_STD;
    txHeader.DLC = 8;
    memcpy(&txData[0], &stats->i2c_errors, 4);
    memcpy(&txData[4], &stats->fram_crc_errors, 4);

    if (HAL_CAN_AddTxMessage(&hcan1, &txHeader, txData, &txMailbox) == HAL_OK)
    {
        stats->can_tx_packets++;
    }
    else
    {
        stats->can_tx_errors++;
    }
}

static uint8_t calculate_checksum(const uint8_t* data, size_t len)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < len; i++)
    {
        crc ^= data[i];
    }
    return crc;
}
