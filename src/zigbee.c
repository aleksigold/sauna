#include <zboss_api.h>
#include <zboss_api_addons.h>
#include <zigbee_helpers.h>
#include <nrf_log.h>

#include "config.h"

typedef struct {
    zb_zcl_basic_attrs_ext_t basic_attr;
    zb_zcl_identify_attrs_t identify_attr;
    zb_zcl_temp_measurement_attrs_t temp_measurement_attr;
} device_ctx_t;

static device_ctx_t device_ctx;
static zb_callback_t callback;

ZB_ZCL_DECLARE_BASIC_ATTRIB_LIST_EXT(
    basic_attr_list,
    &device_ctx.basic_attr.zcl_version,
    &device_ctx.basic_attr.app_version,
    &device_ctx.basic_attr.stack_version,
    &device_ctx.basic_attr.hw_version,
    device_ctx.basic_attr.mf_name,
    device_ctx.basic_attr.model_id,
    device_ctx.basic_attr.date_code,
    &device_ctx.basic_attr.power_source,
    device_ctx.basic_attr.location_id,
    &device_ctx.basic_attr.ph_env,
    device_ctx.basic_attr.sw_ver
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

void zboss_signal_handler(zb_bufid_t bufid) {
    zb_zdo_app_signal_type_t signal_type = zb_get_app_signal(bufid, NULL);
    zb_ret_t status = ZB_GET_APP_SIGNAL_STATUS(bufid);

    switch (signal_type) {
        case ZB_BDB_SIGNAL_DEVICE_REBOOT:
        case ZB_BDB_SIGNAL_STEERING:
            zigbee_default_signal_handler(bufid);

            if (status == RET_OK) {
                callback(0);
            }

            break;
        case ZB_COMMON_SIGNAL_CAN_SLEEP:
            zb_sleep_now();
            break;
        default:
            zigbee_default_signal_handler(bufid);
    }

    if (bufid) {
        zb_buf_free(bufid);
    }
}


void zigbee_init(zb_callback_t func) {
    callback = func;
    zb_ieee_addr_t ieee_addr;

    ZB_SET_TRACE_LEVEL(ZIGBEE_TRACE_LEVEL);
    ZB_SET_TRACE_MASK(ZIGBEE_TRACE_MASK);
    ZB_SET_TRAF_DUMP_OFF();

    ZB_INIT();
    zb_osif_get_ieee_eui64(ieee_addr);
    zb_set_long_address(ieee_addr);
    zb_set_network_ed_role(IEEE_CHANNEL_MASK);
    zigbee_erase_persistent_storage(ZB_FALSE);
    zb_set_rx_on_when_idle(ZB_FALSE);
    zigbee_power_down_unused_ram();
    ZB_AF_REGISTER_DEVICE_CTX(&temperature_context);

    ZB_MEMSET(&device_ctx, 0, sizeof(device_ctx));

    device_ctx.basic_attr.zcl_version = ZB_ZCL_VERSION;
    device_ctx.basic_attr.power_source = ZB_ZCL_BASIC_POWER_SOURCE_BATTERY;
    device_ctx.identify_attr.identify_time = ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE;
    device_ctx.temp_measurement_attr.measure_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_UNKNOWN;
    device_ctx.temp_measurement_attr.min_measure_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_MIN_VALUE_MIN_VALUE;
    device_ctx.temp_measurement_attr.max_measure_value = ZB_ZCL_ATTR_TEMP_MEASUREMENT_MAX_VALUE_MAX_VALUE;
    device_ctx.temp_measurement_attr.tolerance = ZB_ZCL_ATTR_TEMP_MEASUREMENT_TOLERANCE_MAX_VALUE;

    ZB_ZCL_SET_STRING_VAL(
        device_ctx.basic_attr.mf_name,
        MANUFACTURER,
        ZB_ZCL_STRING_CONST_SIZE(MANUFACTURER)
    );

    ZB_ZCL_SET_STRING_VAL(
        device_ctx.basic_attr.model_id,
        MODEL,
        ZB_ZCL_STRING_CONST_SIZE(MODEL)
    );

    zboss_start_no_autostart();
}

void zigbee_loop() {
    zboss_main_loop_iteration();
}

void zigbee_report_attributes(uint16_t temperature) {
    zb_zcl_set_attr_val(
        ENDPOINT,
        ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
        ZB_ZCL_CLUSTER_SERVER_ROLE,
        ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
        (uint8_t *)&temperature,
        ZB_FALSE
    );

    zb_schedule_app_alarm(
        callback,
        temperature,
        REPORTING_INTERVAL * ZB_TIME_ONE_SECOND
    );
}
