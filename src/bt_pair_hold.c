// Copyright (c) 2026
// SPDX-License-Identifier: MIT

#define DT_DRV_COMPAT zmk_behavior_bt_hold

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/ble.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

struct bt_hold_config {
    const uint32_t hold_ms;
    const uint8_t profile;
};

struct bt_hold_data {
    struct k_work_delayable work;
    const struct device *dev;
    bool triggered;
};

static void bt_hold_work_handler(struct k_work *work) {
    struct bt_hold_data *data = CONTAINER_OF(work, struct bt_hold_data, work.work);
    const struct device *dev = data->dev;
    const struct bt_hold_config *cfg = dev->config;

    data->triggered = true;

    int err = zmk_ble_prof_select(cfg->profile);
    if (err) {
        LOG_ERR("Failed to focus profile %u: %d", cfg->profile, err);
        return;
    }

    zmk_ble_clear_bonds();

    err = zmk_ble_prof_select(cfg->profile);
    if (err) {
        LOG_ERR("BT profile %u reselect failed: %d", cfg->profile, err);
    } else {
        LOG_INF("BT profile %u cleared and advertising", cfg->profile);
    }
}

static int bt_hold_press(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct bt_hold_data *data = dev->data;
    const struct bt_hold_config *cfg = dev->config;

    data->triggered = false;
    k_work_reschedule(&data->work, K_MSEC(cfg->hold_ms));

    return ZMK_BEHAVIOR_OPAQUE;
}

static int bt_hold_release(struct zmk_behavior_binding *binding, struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    struct bt_hold_data *data = dev->data;

    if (!data->triggered) {
        k_work_cancel_delayable(&data->work);
    }

    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api bt_hold_driver_api = {
    .binding_pressed = bt_hold_press,
    .binding_released = bt_hold_release,
};

static int bt_hold_init(const struct device *dev) {
    struct bt_hold_data *data = dev->data;

    k_work_init_delayable(&data->work, bt_hold_work_handler);
    data->dev = dev;
    data->triggered = false;

    return 0;
}

#define BT_HOLD_INST(i)                                                                          \
    static struct bt_hold_data bt_hold_data_##i;                                                  \
    static const struct bt_hold_config bt_hold_config_##i = {                                     \
        .hold_ms = DT_INST_PROP(i, hold_ms),                                                      \
        .profile = DT_INST_PROP(i, profile),                                                      \
    };                                                                                            \
    BEHAVIOR_DT_INST_DEFINE(i, bt_hold_init, NULL, &bt_hold_data_##i, &bt_hold_config_##i,        \
                            APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                    \
                            &bt_hold_driver_api);

DT_INST_FOREACH_STATUS_OKAY(BT_HOLD_INST)
