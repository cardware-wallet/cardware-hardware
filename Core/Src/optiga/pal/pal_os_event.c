#include "optiga/pal/pal_os_event.h"
#include "stm32h7xx_hal.h" // Include the HAL header for STM32 timers

// Declare a hardware timer handle (e.g., TIM2)
extern TIM_HandleTypeDef htim1;

// Static instance of the pal_os_event
static pal_os_event_t pal_os_event_0 = {0};

// This function starts the event with a timer
void pal_os_event_start(
    pal_os_event_t *p_pal_os_event,
    register_callback callback,
    void *callback_args
) {
    if (0 == p_pal_os_event->is_event_triggered) {
        p_pal_os_event->is_event_triggered = TRUE;
        pal_os_event_register_callback_oneshot(p_pal_os_event, callback, callback_args, 1000);
    }
}

// This function stops the event
void pal_os_event_stop(pal_os_event_t *p_pal_os_event) {
    p_pal_os_event->is_event_triggered = 0;
}

// This function creates an OS event
pal_os_event_t *pal_os_event_create(register_callback callback, void *callback_args) {
    if ((NULL != callback) && (NULL != callback_args)) {
        pal_os_event_start(&pal_os_event_0, callback, callback_args);
    }
    return (&pal_os_event_0);
}

// This function triggers the registered callback
void pal_os_event_trigger_registered_callback(void) {
    register_callback callback;
    HAL_TIM_Base_Stop_IT(&htim1);
    if (pal_os_event_0.callback_registered) {
        callback = pal_os_event_0.callback_registered;
        callback((void *)pal_os_event_0.callback_ctx);
    }
}

// This function registers a callback to be called after a timer elapses
void pal_os_event_register_callback_oneshot(
    pal_os_event_t *p_pal_os_event,
    register_callback callback,
    void *callback_args,
    uint32_t time_us
) {
    p_pal_os_event->callback_registered = callback;
    p_pal_os_event->callback_ctx = callback_args;

    // Calculate the timer period based on the desired time in microseconds
    uint32_t pclk1_freq = HAL_RCC_GetPCLK1Freq();
    if (pclk1_freq == 0){
    	return; // Clock frequency is zero, abort the timer setup
    }
	//uint32_t timer_period = ((pclk1_freq / 1000000) * time_us) ;
	uint32_t timer_period =  time_us/10 ;


    // Configure and start the timer with the calculated period
    //htim1.Init.Period = time_us*100;
    __HAL_TIM_SET_AUTORELOAD(&htim1, timer_period);
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    HAL_TIM_Base_Start_IT(&htim1);
}

// This function destroys an OS event
void pal_os_event_destroy(pal_os_event_t *pal_os_event) {
    (void)pal_os_event;
    // Stop the hardware timer
    //HAL_TIM_Base_Stop_IT(&htim1);
}

// Timer interrupt callback - Called by HAL when the timer expires

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM1) { // Replace TIM1 with the correct timer instance
    	pal_os_event_0.is_event_triggered = 1;
        pal_os_event_trigger_registered_callback();
    }
}
