#include "optiga/pal/pal_logger.h"
#include "stm32h7xx_hal.h"  // Include STM32 HAL header for UART functions

// Assuming UART1 is used for logging
extern UART_HandleTypeDef huart1;

pal_status_t pal_logger_init(void *p_logger_context) {
    pal_status_t return_status = PAL_STATUS_FAILURE;
    pal_logger_t *p_log_context = (pal_logger_t *)p_logger_context;

    do {
        if (p_log_context == NULL) {
            break;
        }

        // Initialize UART handle here if needed, or ensure it is initialized elsewhere
        p_log_context->logger_config_ptr = &huart1;
        p_log_context->logger_rx_flag = 0;
        p_log_context->logger_tx_flag = 0;

        return_status = PAL_STATUS_SUCCESS;

    } while (0);

    return return_status;
}

pal_status_t pal_logger_deinit(void *p_logger_context) {
    pal_status_t return_status = PAL_STATUS_FAILURE;
    pal_logger_t *p_log_context = (pal_logger_t *)p_logger_context;

    do {
        if (p_log_context == NULL) {
            break;
        }

        // Deinitialize UART handle
        if (HAL_UART_DeInit((UART_HandleTypeDef *)p_log_context->logger_config_ptr) != HAL_OK) {
            break;
        }

        return_status = PAL_STATUS_SUCCESS;

    } while (0);

    return return_status;
}

pal_status_t pal_logger_write(void *p_logger_context, const uint8_t *p_log_data, uint32_t log_data_length) {
    int32_t return_status = PAL_STATUS_FAILURE;
    pal_logger_t *p_log_context = (pal_logger_t *)p_logger_context;

    do {
        if (p_log_context == NULL || p_log_data == NULL) {
            break;
        }

        // Transmit data via UART (blocking mode)
        if (HAL_UART_Transmit((UART_HandleTypeDef *)p_log_context->logger_config_ptr, (uint8_t *)p_log_data, log_data_length, HAL_MAX_DELAY) != HAL_OK) {
            break;
        }

        return_status = PAL_STATUS_SUCCESS;

    } while (0);

    return (pal_status_t)return_status;
}

pal_status_t pal_logger_read(void *p_logger_context, uint8_t *p_log_data, uint32_t log_data_length) {
    int32_t return_status = PAL_STATUS_FAILURE;
    pal_logger_t *p_log_context = (pal_logger_t *)p_logger_context;

    do {
        if (p_log_context == NULL || p_log_data == NULL) {
            break;
        }

        // Receive data via UART (blocking mode)
        if (HAL_UART_Receive((UART_HandleTypeDef *)p_log_context->logger_config_ptr, p_log_data, log_data_length, HAL_MAX_DELAY) != HAL_OK) {
            break;
        }

        return_status = PAL_STATUS_SUCCESS;

    } while (0);

    return (pal_status_t)return_status;
}
