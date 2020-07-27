#include <stdint.h>

void onewire_init();
uint8_t onewire_reset();
void onewire_write(uint8_t byte);
uint8_t onewire_read();
