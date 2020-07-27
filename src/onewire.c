#include <stdint.h>
#include <nrf_gpio.h>
#include <nrf_delay.h>

#define DS_PIN 7

static void high() {
    nrf_gpio_cfg_input(DS_PIN, NRF_GPIO_PIN_PULLUP);
}

static void low() {
    nrf_gpio_cfg_output(DS_PIN);
    nrf_gpio_pin_clear(DS_PIN);
}

static uint8_t read() {
    return nrf_gpio_pin_read(DS_PIN) && 1;
}

void onewire_init() {
    high();
}

uint8_t onewire_reset() {
    uint8_t result;

    low();
    nrf_delay_us(480);
    high();
    nrf_delay_us(60);

    result = read();

    nrf_delay_us(420);

    return result ^ 1;
}

void onewire_write(uint8_t byte) {
    uint8_t i;

    for (i = 0; i < 8; i++) {
        low();
        nrf_delay_us(2);

        if ((byte >> i) & 1) {
            high();
            nrf_delay_us(58);
        } else {
            nrf_delay_us(58);
            high();
        }

        nrf_delay_us(1);
    }
}

uint8_t onewire_read() {
    uint8_t byte = 0;
    uint8_t i;

    for (i = 0; i < 8; i++) {
        low();
        nrf_delay_us(2);
        high();
        nrf_delay_us(8);

        byte |= read() << i;

        nrf_delay_us(50);
    }

    return byte;
}
