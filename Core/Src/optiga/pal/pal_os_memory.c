#include "optiga/pal/pal_os_memory.h"
#include <stdlib.h>  // Required for malloc, calloc, and free
#include <string.h>  // Required for memcpy and memset

/* Malloc definition for PAL OS */
void *pal_os_malloc(uint32_t block_size) {
    return malloc(block_size);
}

/* Calloc definition for PAL OS */
void *pal_os_calloc(uint32_t number_of_blocks, uint32_t block_size) {
    return calloc(number_of_blocks, block_size);
}

/* Free definition for PAL OS */
void pal_os_free(void *p_block) {
    free(p_block);
}

/* Memcpy definition for PAL OS */
void pal_os_memcpy(void *p_destination, const void *p_source, uint32_t size) {
    memcpy(p_destination, p_source, size);
}

/* Memset definition for PAL OS */
void pal_os_memset(void *p_buffer, uint32_t value, uint32_t size) {
    memset(p_buffer, (int)value, size);  // Cast to int for memset function
}
