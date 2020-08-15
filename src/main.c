#include <stdint.h>
#include <app_timer.h>
#include <nrf_log.h>
#include <nrf_log_ctrl.h>
#include <nrf_log_default_backends.h>

#include "ds18b20.h"
#include "zigbee.h"
#include "config.h"

static void report_temperature(uint8_t param) {
    int16_t temperature = ds18b20_read() * 100;

    NRF_LOG_INFO("Temperature: %d", temperature);

    zigbee_report_attributes(temperature);
}

int main() {
    ds18b20_init();
    app_timer_init();
    zigbee_init(report_temperature);

    NRF_LOG_INIT(NULL);
    NRF_LOG_DEFAULT_BACKENDS_INIT();

    while (1) {
        zigbee_loop();
        NRF_LOG_PROCESS();
    }
}
