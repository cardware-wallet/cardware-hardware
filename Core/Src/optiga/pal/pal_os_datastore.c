#include "optiga/pal/pal_os_datastore.h"
#include "stm32h7xx_hal.h"
#include <string.h>  // For memcpy

/// @cond hidden

/// Size of length field
#define LENGTH_SIZE (0x02)
/// Size of data store buffer to hold the shielded connection manage context information (2 bytes length field + 64(0x40) bytes context)
#define MANAGE_CONTEXT_BUFFER_SIZE (0x42)

// Internal buffer to store the shielded connection manage context information (length field + Data)
uint8_t data_store_manage_context_buffer[LENGTH_SIZE + MANAGE_CONTEXT_BUFFER_SIZE];

// Internal buffer to store the optiga application context data during hibernate(length field + Data)
uint8_t data_store_app_context_buffer[LENGTH_SIZE + APP_CONTEXT_SIZE];

// Internal buffer to store the generated platform binding shared secret on Host (length field + shared secret)
uint8_t optiga_platform_binding_shared_secret[LENGTH_SIZE + OPTIGA_SHARED_SECRET_MAX_LENGTH] = {
    0x00, 0x40,  // Length of the shared secret, followed after the length information
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
    0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
    0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30,
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40
};

pal_status_t
pal_os_datastore_write(uint16_t datastore_id, const uint8_t *p_buffer, uint16_t length) {
    pal_status_t return_status = PAL_STATUS_FAILURE;
    uint8_t offset = 0;

    switch (datastore_id) {
        case OPTIGA_PLATFORM_BINDING_SHARED_SECRET_ID: {
            // Storing platform binding shared secret in RAM
            if (length <= OPTIGA_SHARED_SECRET_MAX_LENGTH) {
                optiga_platform_binding_shared_secret[offset++] = (uint8_t)(length >> 8);
                optiga_platform_binding_shared_secret[offset++] = (uint8_t)(length);
                memcpy(&optiga_platform_binding_shared_secret[offset], p_buffer, length);
                return_status = PAL_STATUS_SUCCESS;
            }
            break;
        }
        case OPTIGA_COMMS_MANAGE_CONTEXT_ID: {
            // Store the manage context in RAM or NVM
            data_store_manage_context_buffer[offset++] = (uint8_t)(length >> 8);
            data_store_manage_context_buffer[offset++] = (uint8_t)(length);
            memcpy(&data_store_manage_context_buffer[offset], p_buffer, length);
            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        case OPTIGA_HIBERNATE_CONTEXT_ID: {
            // Store the application context in RAM or NVM
            data_store_app_context_buffer[offset++] = (uint8_t)(length >> 8);
            data_store_app_context_buffer[offset++] = (uint8_t)(length);
            memcpy(&data_store_app_context_buffer[offset], p_buffer, length);
            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        default: {
            break;
        }
    }
    return return_status;
}

pal_status_t
pal_os_datastore_read(uint16_t datastore_id, uint8_t *p_buffer, uint16_t *p_buffer_length) {
    pal_status_t return_status = PAL_STATUS_FAILURE;
    uint16_t data_length;
    uint8_t offset = 0;

    switch (datastore_id) {
        case OPTIGA_PLATFORM_BINDING_SHARED_SECRET_ID: {
            // Read from RAM or NVM
            data_length = (uint16_t)(optiga_platform_binding_shared_secret[offset++] << 8);
            data_length |= (uint16_t)(optiga_platform_binding_shared_secret[offset++]);
            if (data_length <= OPTIGA_SHARED_SECRET_MAX_LENGTH) {
                memcpy(p_buffer, &optiga_platform_binding_shared_secret[offset], data_length);
                *p_buffer_length = data_length;
                return_status = PAL_STATUS_SUCCESS;
            }
            break;
        }
        case OPTIGA_COMMS_MANAGE_CONTEXT_ID: {
            // Read from RAM or NVM
            data_length = (uint16_t)(data_store_manage_context_buffer[offset++] << 8);
            data_length |= (uint16_t)(data_store_manage_context_buffer[offset++]);
            memcpy(p_buffer, &data_store_manage_context_buffer[offset], data_length);
            *p_buffer_length = data_length;
            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        case OPTIGA_HIBERNATE_CONTEXT_ID: {
            // Read from RAM or NVM
            data_length = (uint16_t)(data_store_app_context_buffer[offset++] << 8);
            data_length |= (uint16_t)(data_store_app_context_buffer[offset++]);
            memcpy(p_buffer, &data_store_app_context_buffer[offset], data_length);
            *p_buffer_length = data_length;
            return_status = PAL_STATUS_SUCCESS;
            break;
        }
        default: {
            *p_buffer_length = 0;
            break;
        }
    }

    return return_status;
}
/// @endcond
/**
 * @}
 */
