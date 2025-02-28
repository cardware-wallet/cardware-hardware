#include "optiga/pal/pal_i2c.h"
#include "stm32h7xx_hal.h"// Include STM32 HAL library

#define PAL_I2C_MASTER_MAX_BITRATE (100)

extern I2C_HandleTypeDef hi2c2;  // Reference to the I2C handle

static volatile uint32_t g_entry_count = 0;
static pal_i2c_t *gp_pal_i2c_current_ctx;

#define MAX_I2C_RETRY 0
#define RETRY_DELAY_MS 10

static pal_status_t pal_i2c_acquire(const void *p_i2c_context) {
    (void)p_i2c_context;

    if (0 == g_entry_count) {
        g_entry_count++;
        if (1 == g_entry_count) {
            return PAL_STATUS_SUCCESS;
        }
    }
    return PAL_STATUS_FAILURE;
}

static void pal_i2c_release(const void *p_i2c_context) {
    (void)p_i2c_context;
    g_entry_count = 0;
}

void invoke_upper_layer_callback(const pal_i2c_t *p_pal_i2c_ctx, optiga_lib_status_t event) {
    upper_layer_callback_t upper_layer_handler;

    upper_layer_handler = (upper_layer_callback_t)p_pal_i2c_ctx->upper_layer_event_handler;
    upper_layer_handler(p_pal_i2c_ctx->p_upper_layer_ctx, event);

    pal_i2c_release(p_pal_i2c_ctx->p_upper_layer_ctx);
}

// Implement I2C callback functions for interrupt-based communication
void i2c_master_end_of_transmit_callback(void) {
    invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_SUCCESS);
}

void i2c_master_end_of_receive_callback(void) {
    invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_SUCCESS);
}

void i2c_master_error_detected_callback(void) {
    invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_ERROR);
}

void i2c_master_nack_received_callback(void) {
    i2c_master_error_detected_callback();
}

void i2c_master_arbitration_lost_callback(void) {
    i2c_master_error_detected_callback();
}

pal_status_t pal_i2c_init(const pal_i2c_t *p_i2c_context) {
    if (p_i2c_context != NULL && p_i2c_context->p_i2c_hw_config != NULL) {
        // Initialization is handled by HAL_MspInit() in STM32 HAL. No additional init required.
    	//HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5, GPIO_PIN_RESET);
    	//HAL_Delay(15);
    	//HAL_GPIO_WritePin(GPIOB,GPIO_PIN_5, GPIO_PIN_SET);
    	//HAL_Delay(10);
        return PAL_STATUS_SUCCESS;
    }
    return PAL_STATUS_FAILURE;
}

pal_status_t pal_i2c_deinit(const pal_i2c_t *p_i2c_context) {
    if (p_i2c_context != NULL && p_i2c_context->p_i2c_hw_config != NULL) {
        HAL_I2C_DeInit(&hi2c2); // Deinitialize the I2C hardware
        return PAL_STATUS_SUCCESS;
    }
    return PAL_STATUS_FAILURE;
}

pal_status_t pal_i2c_write(const pal_i2c_t *p_i2c_context, uint8_t *p_data, uint16_t length) {
    pal_status_t status = PAL_STATUS_FAILURE;

    // Acquire the I2C bus before performing the write operation
    if (PAL_STATUS_SUCCESS == pal_i2c_acquire(p_i2c_context)) {
        gp_pal_i2c_current_ctx = (pal_i2c_t *)p_i2c_context;

        // Loop to handle HAL_BUSY status
        HAL_StatusTypeDef hal_status;
        //int retry_count = 0;

	  hal_status = HAL_I2C_Master_Transmit(
		  (I2C_HandleTypeDef *)p_i2c_context->p_i2c_hw_config,
		  (p_i2c_context->slave_address << 1),
		  p_data,
		  length,
		  HAL_MAX_DELAY);

	  if (hal_status == HAL_OK) {
		  invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_SUCCESS);
		  status = PAL_STATUS_SUCCESS;
	  } else {
		  // Handle HAL_ERROR or HAL_TIMEOUT
		  invoke_upper_layer_callback(p_i2c_context, PAL_I2C_EVENT_ERROR);
		  pal_i2c_release((void *)p_i2c_context);
	  }
    } else {
        // I2C bus is busy
        status = PAL_STATUS_I2C_BUSY;
        ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler)
         )(p_i2c_context->p_upper_layer_ctx, PAL_I2C_EVENT_BUSY);
    }

    return status;
}


pal_status_t pal_i2c_read(const pal_i2c_t *p_i2c_context, uint8_t *p_data, uint16_t length) {
    pal_status_t status = PAL_STATUS_FAILURE;

    if (PAL_STATUS_SUCCESS == pal_i2c_acquire(p_i2c_context)) {
    	gp_pal_i2c_current_ctx = (pal_i2c_t *)p_i2c_context;

        HAL_StatusTypeDef hal_status = HAL_I2C_Master_Receive(
            (I2C_HandleTypeDef *)p_i2c_context->p_i2c_hw_config,
            (p_i2c_context->slave_address << 1),
            p_data,
            length,
            HAL_MAX_DELAY);

        if (hal_status == HAL_OK) {
            invoke_upper_layer_callback(gp_pal_i2c_current_ctx, PAL_I2C_EVENT_SUCCESS);
            status = PAL_STATUS_SUCCESS;
        } else {
            invoke_upper_layer_callback(p_i2c_context, PAL_I2C_EVENT_ERROR);
            pal_i2c_release((void *)p_i2c_context);
        }
    } else {
        status = PAL_STATUS_I2C_BUSY;
        ((upper_layer_callback_t)(p_i2c_context->upper_layer_event_handler)
        )(p_i2c_context->p_upper_layer_ctx, PAL_I2C_EVENT_BUSY);
    }
    return status;
}

pal_status_t pal_i2c_set_bitrate(const pal_i2c_t *p_i2c_context, uint16_t bitrate) {
    pal_status_t return_status = PAL_STATUS_FAILURE;
    optiga_lib_status_t event = PAL_I2C_EVENT_ERROR;

    if (PAL_STATUS_SUCCESS == pal_i2c_acquire(p_i2c_context)) {
        if (bitrate > PAL_I2C_MASTER_MAX_BITRATE) {
            bitrate = PAL_I2C_MASTER_MAX_BITRATE;
        }
        
        //hi2c2.Init.ClockSpeed = bitrate * 1000; // Convert to Hz
        if (HAL_I2C_Init(&hi2c2) == HAL_OK) {
            return_status = PAL_STATUS_SUCCESS;
            event = PAL_I2C_EVENT_SUCCESS;
        } else {
            return_status = PAL_STATUS_FAILURE;
        }
    } else {
        return_status = PAL_STATUS_I2C_BUSY;
        event = PAL_I2C_EVENT_BUSY;
    }

    if (0 != p_i2c_context->upper_layer_event_handler) {
        ((callback_handler_t)(p_i2c_context->upper_layer_event_handler)
        )(p_i2c_context->p_upper_layer_ctx, event);
    }

    if (PAL_STATUS_I2C_BUSY != return_status) {
        pal_i2c_release((void *)p_i2c_context);
    }
    return return_status;
}
