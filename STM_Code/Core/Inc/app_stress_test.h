/*
 * app_stress_test.h
 *
 *  Created on: 12.06.2026
 *      Author: Gemini
 */

#ifndef INC_APP_STRESS_TEST_H_
#define INC_APP_STRESS_TEST_H_

#include "stm32f4xx_hal.h"

// Configuration for the stress test
#define STRESS_CPU_SLICE_US         1000  // Duration of a CPU stress slice in microseconds
#define STRESS_FRAM_TEST_INTERVAL_MS 50   // Interval for FRAM read/write tests
#define STRESS_OLED_UPDATE_INTERVAL_MS 250 // Interval for OLED display updates

void stress_test_init(void);
void stress_test_run_slice(void);


#endif /* INC_APP_STRESS_TEST_H_ */
