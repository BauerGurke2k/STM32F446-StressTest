/*
 * app_stress_test.c
 *
 *  Created on: 12.06.2026
 *      Author: Gemini
 */

#include "app_stress_test.h"
#include "app_stats.h"
#include "main.h" // For LED access and HAL handles
#include "oled.h" // Assuming oled functions exist
#include "fram.h" // Assuming fram functions exist
#include <stdio.h>
#include <string.h>


// DWT cycle counter for microsecond timing
#define DWT_CONTROL *(volatile uint32_t*)0xE0001000
#define DWT_CYCCNT  *(volatile uint32_t*)0xE0001004
#define CPU_FREQUENCY_MHZ (SystemCoreClock / 1000000)

// --- Private function prototypes ---
static void stress_cpu_slice(void);
static void stress_fram_test(void);
static void stress_oled_update(void);
static uint32_t crc32_compute(const uint8_t *p_data, uint32_t size, uint32_t *p_crc);

// --- Private variables ---
static volatile uint32_t a_cpu_stress_dummy = 0;
static uint8_t fram_test_buffer[32];
static uint32_t fram_test_address = 0;


void stress_test_init(void)
{
    // Enable DWT Cycle Counter for timing
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT_CONTROL |= 1;
    DWT_CYCCNT = 0;
}

void stress_test_run_slice(void)
{
    static uint32_t last_fram_test_ms = 0;
    static uint32_t last_oled_update_ms = 0;
    app_stats_t* stats = stats_get();

    // 1. CPU Stress part
    stress_cpu_slice();

    // 2. FRAM Test part (periodic)
    if (stats->uptime_ms - last_fram_test_ms >= STRESS_FRAM_TEST_INTERVAL_MS)
    {
        last_fram_test_ms = stats->uptime_ms;
        stress_fram_test();
    }

    // 3. OLED Update part (periodic)
    if (stats->uptime_ms - last_oled_update_ms >= STRESS_OLED_UPDATE_INTERVAL_MS)
    {
        last_oled_update_ms = stats->uptime_ms;
        stress_oled_update();
    }
}

static void stress_cpu_slice(void)
{
    app_stats_t* stats = stats_get();
    uint32_t start_cycles = DWT_CYCCNT;
    uint32_t target_cycles = STRESS_CPU_SLICE_US * CPU_FREQUENCY_MHZ;

    // Perform some calculations until the time slice is over
    // Using a simple pseudo-random number generator as load
    uint32_t lfsr = 0xACE1u;
    do {
        lfsr = (lfsr >> 1) ^ (-(lfsr & 1u) & 0xB400u); // 16-bit CRC-like LFSR
        a_cpu_stress_dummy += lfsr; // Ensure compiler doesn't optimize it out
        stats->cpu_stress_iterations++;
    } while ((DWT_CYCCNT - start_cycles) < target_cycles);
}

static void stress_fram_test(void)
{
    app_stats_t* stats = stats_get();
    stats->i2c_transactions++;
    stats->fram_write_count++;

    // 1. Prepare test data
    uint32_t seq = stats->fram_write_count;
    memcpy(fram_test_buffer, &seq, sizeof(seq));
    // Fill rest of buffer with some pattern
    for (size_t i = sizeof(seq); i < sizeof(fram_test_buffer) - 4; ++i) {
        fram_test_buffer[i] = (uint8_t)(seq + i);
    }

    // 2. Calculate CRC and append it
    uint32_t crc_val = 0;
    crc32_compute(fram_test_buffer, sizeof(fram_test_buffer) - 4, &crc_val);
    memcpy(&fram_test_buffer[sizeof(fram_test_buffer) - 4], &crc_val, sizeof(crc_val));

    // 3. Write to FRAM
    if (fram_write(fram_test_address, fram_test_buffer, sizeof(fram_test_buffer)) != HAL_OK)
    {
        stats->i2c_errors++;
        return;
    }

    // 4. Read back from FRAM
    uint8_t read_buffer[sizeof(fram_test_buffer)];
    stats->i2c_transactions++;
    stats->fram_read_count++;
    if (fram_read(fram_test_address, read_buffer, sizeof(read_buffer)) != HAL_OK)
    {
        stats->i2c_errors++;
        return;
    }

    // 5. Verify CRC
    uint32_t new_crc_val = 0;
    crc32_compute(read_buffer, sizeof(read_buffer) - 4, &new_crc_val);
    uint32_t read_crc;
    memcpy(&read_crc, &read_buffer[sizeof(read_buffer) - 4], sizeof(read_crc));

    if (new_crc_val != read_crc)
    {
        stats->fram_crc_errors++;
    }

    // 6. Move to next address for next test
    fram_test_address = (fram_test_address + sizeof(fram_test_buffer)) % (FRAM_SIZE_BYTES - sizeof(fram_test_buffer));
}


static void stress_oled_update(void)
{
    app_stats_t* stats = stats_get();
    char buffer[40];

    oled_SetCursor(0, 0);
    sprintf(buffer, "UP %lus", stats->uptime_s);
    oled_WriteString(buffer, Font_7x10, White);

    oled_SetCursor(0, 12);
    sprintf(buffer, "USB %lu/%lu", stats->usb_tx_packets, stats->usb_tx_busy);
    oled_WriteString(buffer, Font_7x10, White);

    oled_SetCursor(0, 24);
    sprintf(buffer, "CAN %lu/%lu/%lu", stats->can_tx_packets, stats->can_rx_packets, stats->can_tx_errors + stats->can_busoff_count);
    oled_WriteString(buffer, Font_7x10, White);

    oled_SetCursor(0, 36);
    sprintf(buffer, "I2C/FR %lu/%lu", stats->i2c_errors, stats->fram_crc_errors);
    oled_WriteString(buffer, Font_7x10, White);

    oled_SetCursor(0, 48);
     if (stats->i2c_errors > 0 || stats->fram_crc_errors > 0 || stats->can_busoff_count > 0 || stats->usb_tx_busy > 100) {
        sprintf(buffer, "STAT: WARN");
    } else {
        sprintf(buffer, "STAT: PASS");
    }
    oled_WriteString(buffer, Font_7x10, White);

    oled_UpdateScreen();
    stats->oled_update_count++;
}


// Standard STM32 HAL CRC32 implementation
static uint32_t crc32_compute(const uint8_t *p_data, uint32_t size, uint32_t *p_crc)
{
    // This is a software fallback. If CubeMX has CRC peripheral enabled,
    // it's better to use HAL_CRC_Calculate().
    static uint32_t crc_table[256];
    static int have_table = 0;
    uint32_t rem = 0xFFFFFFFF;
    const uint8_t *p;
    int i, j;

    if (have_table == 0) {
        for (i = 0; i < 256; i++) {
            rem = i;
            for (j = 0; j < 8; j++) {
                if (rem & 1) {
                    rem >>= 1;
                    rem ^= 0xedb88320;
                } else
                    rem >>= 1;
            }
            crc_table[i] = rem;
        }
        have_table = 1;
    }

    rem = *p_crc;
    for (p = p_data; size > 0; size--) {
        rem = (rem >> 8) ^ crc_table[(rem & 0xFF) ^ *p++];
    }

    *p_crc = rem;
    return *p_crc;
}
