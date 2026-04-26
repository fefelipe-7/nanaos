#include "drivers/block/block.h"
#include "log/logger.h"

static block_device_t *primary_dev = NULL;

void block_register(block_device_t *dev) {
    primary_dev = dev;
    if (dev && dev->name) {
        log_info("[BLOCK] device registered");
    }
}

block_device_t *block_get_primary(void) {
    return primary_dev;
}
