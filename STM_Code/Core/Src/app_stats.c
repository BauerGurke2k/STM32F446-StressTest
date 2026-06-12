/*
 * app_stats.c
 *
 *  Created on: 12.06.2026
 *      Author: Gemini
 */

#include "app_stats.h"
#include <string.h> // For memset

static app_stats_t g_stats;

void stats_init(void)
{
    memset(&g_stats, 0, sizeof(app_stats_t));
    stats_check_reset_reason();
}

app_stats_t* stats_get(void)
{
    return &g_stats;
}

void stats_reset(void)
{
    // Note: Don't reset everything, some counters might be persistent
    // across soft resets if needed. For now, we clear most things.
    uint32_t hardfaults = g_stats.hardfault_count;
    reset_reason_t reason = g_stats.reset_reason;

    memset(&g_stats, 0, sizeof(app_stats_t));

    g_stats.hardfault_count = hardfaults;
    g_stats.reset_reason = reason;
}

void stats_update_loop_time(uint32_t loop_time_us)
{
    if (loop_time_us > g_stats.max_loop_time_us)
    {
        g_stats.max_loop_time_us = loop_time_us;
    }
}

void stats_check_reset_reason(void)
{
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))
    {
        g_stats.reset_reason = RESET_REASON_LOW_POWER;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))
    {
        g_stats.reset_reason = RESET_REASON_WWDG;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
    {
        g_stats.reset_reason = RESET_REASON_IWDG;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
    {
        g_stats.reset_reason = RESET_REASON_SOFTWARE;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))
    {
        g_stats.reset_reason = RESET_REASON_PIN;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
    {
        g_stats.reset_reason = RESET_REASON_POWER_ON;
    }
    else
    {
        g_stats.reset_reason = RESET_REASON_UNKNOWN;
    }
    // Clear the reset flags
    __HAL_RCC_CLEAR_RESET_FLAGS();
}
