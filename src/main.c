#include <zboss_api.h>
#include <zboss_api_addons.h>
#include <zigbee_helpers.h>

#if !defined ZB_ED_ROLE
#error Define ZB_ED_ROLE to compile End Device source code.
#endif

#define REPORTING_INTERVAL 5
#define ENDPOINT 1
#define IEEE_CHANNEL_MASK (1l << ZIGBEE_CHANNEL)

typedef struct {
    zb_zcl_basic_attrs_t basic_attr;
    zb_zcl_identify_attrs_t identify_attr;
    zb_zcl_temp_measurement_attrs_t temp_measurement_attr;
} device_ctx_t;

static device_ctx_t device_ctx;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST(
    basic_attr_list,
    &device_ctx.basic_attr.zcl_version,
    &device_ctx.basic_attr.power_source
);

ZB_ZCL_DECLARE_IDENTIFY_ATTRIB_LIST(
    identify_attr_list,
    &device_ctx.identify_attr.identify_time
);

ZB_ZCL_DECLARE_TEMP_MEASUREMENT_ATTRIB_LIST(
    temperature_attr_list,
    &device_ctx.temp_measurement_attr.measure_value,
    &device_ctx.temp_measurement_attr.min_measure_value,
    &device_ctx.temp_measurement_attr.max_measure_value,
    &device_ctx.temp_measurement_attr.tolerance
);

ZB_HA_DECLARE_TEMPERATURE_SENSOR_CLUSTER_LIST(
    temperature_clusters,
    basic_attr_list,
    identify_attr_list,
    temperature_attr_list
);

ZB_HA_DECLARE_TEMPERATURE_SENSOR_EP(
    temperature_endpoint,
    ENDPOINT,
    temperature_clusters
);

ZB_HA_DECLARE_TEMPERATURE_SENSOR_CTX(temperature_context, temperature_endpoint);

static void report_temperature(zb_uint8_t param) {
    static zb_int16_t temperature = 2500;

    ZB_ZCL_SET_ATTRIBUTE(
        ENDPOINT,
        ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
        (zb_uint8_t *)&temperature,
        ZB_FALSE
    );

    ZB_SCHEDULE_APP_ALARM(
        report_temperature,
        temperature,
        REPORTING_INTERVAL * ZB_TIME_ONE_SECOND
    );
}

void zboss_signal_handler(zb_bufid_t bufid) {
    zb_zdo_app_signal_type_t signal_type = zb_get_app_signal(bufid, NULL);
    zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(bufid);

    switch (signal_type) {
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        case ZB_BDB_SIGNAL_STEERING:
            zigbee_default_signal_handler(bufid);

            if (status == RET_OK) {
                report_temperature(0);
            }

            break;
        default:
            zigbee_default_signal_handler(bufid);
    }

    if (bufid) {
        zb_buf_free(bufid);
    }
}

void init_attrs() {
    ZB_MEMSET(&device_ctx, 0, sizeof(device_ctx));

    device_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
    device_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_BATTERY;
    device_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
    device_ctx.temp_measurement_attr.measure_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_UNKNOWN;
    device_ctx.temp_measurement_attr.min_measure_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_MIN_VALUE_MIN_VALUE;
    device_ctx.temp_measurement_attr.max_measure_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_MAX_VALUE_MAX_VALUE;
    device_ctx.temp_measurement_attr.tolerance = ZB_ZCL_ATTR_TEMP_MEASUREMENT_TOLERANCE_MAX_VALUE;
}

void setup() {
    zb_ieee_addr_t ieee_addr;

    app_timer_init();

    ZB_INIT();
    zb_osif_get_ieee_eui64(ieee_addr);
    zb_set_long_address(ieee_addr);
    zb_set_network_ed_role(IEEE_CHANNEL_MASK);
    zigbee_erase_persistent_storage(ZB_FALSE);
    zb_set_ed_timeout(ED_AGING_TIMEOUT_64MIN);
    zb_set_keepalive_timeout(ZB_MILLISECONDS_TO_BEACON_INTERVAL(3000));
    zb_set_rx_on_when_idle(ZB_FALSE);
    ZB_AF_REGISTER_DEVICE_CTX(&temperature_context);

    init_attrs();

    zboss_start_no_autostart();
}

void loop() {
    while (1) {
        zboss_main_loop_iteration();
    }
}

int main() {
    setup();
    loop();
}
