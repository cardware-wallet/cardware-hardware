#include "optiga/pal/pal_gpio.h"
#include "stm32h7xx_hal.h" // Include STM32 HAL library

pal_status_t pal_gpio_init(const pal_gpio_t *p_gpio_context) {
    if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL)) {
        GPIO_InitTypeDef GPIO_InitStruct = {0};
        const stm32_gpio_t* gpio = (stm32_gpio_t*)p_gpio_context->p_gpio_hw;
        // Configure the GPIO pin (assuming p_gpio_hw is a pointer to the GPIO port)
        GPIO_InitStruct.Pin = gpio->pin; // Extract pin number
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;  // Configure as push-pull output
        GPIO_InitStruct.Pull = GPIO_NOPULL;          // No pull-up or pull-down resistor
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // Set low speed

        HAL_GPIO_Init(gpio->port, &GPIO_InitStruct);
        return PAL_STATUS_SUCCESS;
    }
    return PAL_STATUS_FAILURE;
}

pal_status_t pal_gpio_deinit(const pal_gpio_t *p_gpio_context) {
    if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL)) {
        const stm32_gpio_t* gpio = (stm32_gpio_t*)p_gpio_context->p_gpio_hw;
        HAL_GPIO_DeInit(gpio->port, gpio->pin); // De-initialize GPIO pin
        return PAL_STATUS_SUCCESS;
    }
    return PAL_STATUS_FAILURE;
}

void pal_gpio_set_high(const pal_gpio_t *p_gpio_context) {
    if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL)) {
        const stm32_gpio_t* gpio = (stm32_gpio_t*)p_gpio_context->p_gpio_hw;
        //GPIO_TypeDef *port = GPIOB;
        //uint32_t pin = GPIO_PIN_5;
        HAL_GPIO_WritePin(gpio->port,gpio->pin, GPIO_PIN_SET); // Set GPIO pin high
    }
}

void pal_gpio_set_low(const pal_gpio_t *p_gpio_context) {
    if ((p_gpio_context != NULL) && (p_gpio_context->p_gpio_hw != NULL)) {
        //uint32_t pin = (uint16_t)((uint32_t)p_gpio_context->p_gpio_hw & 0xFFFF);
        // *port = (GPIO_TypeDef *)((uint32_t)p_gpio_context->p_gpio_hw & 0xFFFF0000);
    	const stm32_gpio_t* gpio = (stm32_gpio_t*)p_gpio_context->p_gpio_hw;
        //GPIO_TypeDef *port = GPIOB;
        //uint32_t pin = GPIO_PIN_5;
        HAL_GPIO_WritePin(gpio->port,gpio->pin, GPIO_PIN_RESET); // Set GPIO pin low
    }
}
