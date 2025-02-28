#include "optiga/pal/pal_os_timer.h"
#include "stm32h7xx_hal.h"  // Include STM32 HAL header for timing functions

// Variable to track the tick count (in milliseconds)
static volatile uint32_t g_tick_count  = 0;

uint32_t pal_os_timer_get_time_in_microseconds(void) {
    // Use HAL_GetTick() to get time in milliseconds and convert to microseconds
    uint32_t tick_in_us = HAL_GetTick() * 1000;
    return tick_in_us;
    //static uint32_t count = 0;
        // The implementation must ensure that every invocation of this API returns a unique value.
    //return (count++);
}

uint32_t pal_os_timer_get_time_in_milliseconds(void) {
    // Use HAL_GetTick() to return time in milliseconds
	g_tick_count = HAL_GetTick();
	return g_tick_count;
}

void pal_os_timer_delay_in_milliseconds(uint16_t milliseconds) {
    // Use HAL_Delay() to create a delay in milliseconds
    HAL_Delay(milliseconds);
	/*
	uint32_t start_time;
	uint32_t current_time;
	uint32_t time_stamp_diff;

	start_time = pal_os_timer_get_time_in_milliseconds();
	current_time = start_time;
	time_stamp_diff = current_time - start_time;
	while (time_stamp_diff <= (uint32_t)milliseconds){
		current_time = pal_os_timer_get_time_in_milliseconds();
	    time_stamp_diff = current_time - start_time;
	    if (start_time > current_time){
	    	time_stamp_diff = (0xFFFFFFFF + (current_time - start_time)) + 0x01;
	    }
	}*/
}

pal_status_t pal_timer_init(void) {
    // Timer initialization is already handled by HAL, so return success
    return PAL_STATUS_SUCCESS;
}

pal_status_t pal_timer_deinit(void) {
    // No specific de-initialization required for HAL timer, so return success
    return PAL_STATUS_SUCCESS;
}
