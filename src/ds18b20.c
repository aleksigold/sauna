#include <stdint.h>

#include "onewire.h"

void ds18b20_init() {
    onewire_init();
}

float ds18b20_read() {
    uint16_t raw;

    uint8_t buf[2];
    uint8_t i;

    onewire_reset();
    onewire_write(0xCC);
    onewire_write(0x44);

    while (onewire_read() == 0) {}

    onewire_reset();
    onewire_write(0xCC);
    onewire_write(0xBE);

    for (i = 0; i < 2; i++) {
        buf[i] = onewire_read();
    }

    raw = buf[0] | (buf[1] << 8);

    return (raw >> 11 ? -1 : 1) * (raw & ~(0b11111 << 11)) * 0.0625;
}
