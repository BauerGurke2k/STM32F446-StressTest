/*
 * app_stats.h
 *
 *  Created on: 12.06.2026
 *      Author: Gemini
 */

#ifndef INC_APP_STATS_H_
#define INC_APP_STATS_H_

#include "stm32f4xx_hal.h"
#include <stdint.h>

// Reset Reason (optional, basic implementation)
typedef enum {
    RESET_REASON_UNKNOWN = 0,
    RESET_REASON_POWER_ON,
    RESET_REASON_PIN,
    RESET_REASON_SOFTWARE,
    RESET_REASON_IWDG,
    RESET_REASON_WWDG,
    RESET_REASON_LOW_POWER,
} reset_reason_t;

// Main statistics structure
typedef struct {
    // Uptime
    volatile uint32_t uptime_ms;
    volatile uint32_t uptime_s;

    // Main loop performance
    volatile uint32_t main_loop_counter;
    volatile uint32_t max_loop_time_us;

    // CPU Stress
    volatile uint64_t cpu_stress_iterations;

    // USB CDC
    volatile uint32_t usb_tx_packets;
    volatile uint32_t usb_rx_packets;
    volatile uint32_t usb_tx_busy;
    volatile uint32_t usb_rx_crc_errors;
    volatile uint32_t usb_rx_seq_errors;

    // CAN
    volatile uint32_t can_tx_packets;
    volatile uint32_t can_rx_packets;
    volatile uint32_t can_tx_errors;
    volatile uint32_t can_rx_seq_errors;
    volatile uint32_t can_busoff_count;
    volatile uint32_t can_error_warning_count;
    volatile uint32_t can_error_passive_count;

    // I2C & Peripherals
    volatile uint32_t i2c_transactions;
    volatile uint32_t i2c_errors;
    volatile uint32_t fram_write_count;
    volatile uint32_t fram_read_count;
    volatile uint32_t fram_crc_errors;
    volatile uint32_t oled_update_count;

    // System Health
    reset_reason_t reset_reason;
    volatile uint32_t hardfault_count;

} app_stats_t;


void stats_init(void);
app_stats_t* stats_get(void);
void stats_reset(void);
void stats_update_loop_time(uint32_t loop_time_us);
void stats_check_reset_reason(void);


#endif /* INC_APP_STATS_H_ */
