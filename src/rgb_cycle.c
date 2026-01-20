// Copyright (c) 2026
// SPDX-License-Identifier: MIT

#define DT_DRV_COMPAT zmk_behavior_rgb_cycle

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/rgb_underglow.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

enum rgb_cycle_mode {
    RGB_CYCLE_MODE_BRIGHTNESS,
    RGB_CYCLE_MODE_SPEED,
};

struct rgb_cycle_config {
    enum rgb_cycle_mode mode;
};

struct rgb_cycle_data {
    uint8_t speed_index;
    bool speed_initialized;
};

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)

static int cycle_brightness(void) {
    struct zmk_led_hsb current = zmk_rgb_underglow_calc_brt(0);

    if (current.b >= CONFIG_ZMK_RGB_UNDERGLOW_BRT_MAX) {
        int steps = DIV_ROUND_UP((int)current.b - CONFIG_ZMK_RGB_UNDERGLOW_BRT_MIN,
                                 CONFIG_ZMK_RGB_UNDERGLOW_BRT_STEP);
        if (steps <= 0) {
            steps = 1;
        }
        return zmk_rgb_underglow_change_brt(-steps);
    }

    return zmk_rgb_underglow_change_brt(1);
}

#define RGB_SPEED_MIN 1
#define RGB_SPEED_MAX 5

static int apply_speed(uint8_t target) {
    // Normalize to the minimum speed to get deterministic wrap-around behaviour.
    for (int i = 0; i < RGB_SPEED_MAX; i++) {
        int err = zmk_rgb_underglow_change_spd(-1);
        if (err < 0) {
            return err;
        }
    }

    for (uint8_t i = RGB_SPEED_MIN; i < target; i++) {
        int err = zmk_rgb_underglow_change_spd(1);
        if (err < 0) {
            return err;
        }
    }

    return 0;
}

static int cycle_speed(struct rgb_cycle_data *data) {
    if (!data->speed_initialized) {
        data->speed_index = CLAMP(CONFIG_ZMK_RGB_UNDERGLOW_SPD_START, RGB_SPEED_MIN, RGB_SPEED_MAX);
        data->speed_initialized = true;
    }

    data->speed_index++;
    if (data->speed_index > RGB_SPEED_MAX) {
        data->speed_index = RGB_SPEED_MIN;
    }

    return apply_speed(data->speed_index);
}

#endif // CONFIG_ZMK_RGB_UNDERGLOW

static int rgb_cycle_pressed(struct zmk_behavior_binding *binding,
                             struct zmk_behavior_binding_event event) {
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct rgb_cycle_config *cfg = dev->config;
    struct rgb_cycle_data *data = dev->data;

#if !IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
    ARG_UNUSED(data);
    ARG_UNUSED(event);
    LOG_WRN("RGB cycle behavior invoked without underglow enabled");
    return -ENOTSUP;
#else
    int rc = 0;

    switch (cfg->mode) {
    case RGB_CYCLE_MODE_BRIGHTNESS:
        rc = cycle_brightness();
        break;
    case RGB_CYCLE_MODE_SPEED:
        rc = cycle_speed(data);
        break;
    default:
        rc = -ENOTSUP;
        break;
    }

    if (rc < 0) {
        LOG_ERR("RGB cycle behavior failed: %d", rc);
    }

    return rc;
#endif
}

static int rgb_cycle_released(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api rgb_cycle_driver_api = {
    .binding_pressed = rgb_cycle_pressed,
    .binding_released = rgb_cycle_released,
};

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)

#define RGB_CYCLE_INST(n)                                                                          \
    static struct rgb_cycle_data rgb_cycle_data_##n = {                                            \
        .speed_index = 0,                                                                          \
        .speed_initialized = false,                                                                \
    };                                                                                             \
    static const struct rgb_cycle_config rgb_cycle_config_##n = {                                  \
        .mode = DT_INST_ENUM_IDX(n, mode),                                                         \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, &rgb_cycle_data_##n, &rgb_cycle_config_##n,             \
                            APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,                     \
                            &rgb_cycle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RGB_CYCLE_INST)

#else

static int rgb_cycle_init_stub(const struct device *dev) {
    ARG_UNUSED(dev);
    return 0;
}

#define RGB_CYCLE_INST(n)                                                                          \
    static struct rgb_cycle_data rgb_cycle_data_##n;                                               \
    static const struct rgb_cycle_config rgb_cycle_config_##n = {                                  \
        .mode = DT_INST_ENUM_IDX(n, mode),                                                         \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, rgb_cycle_init_stub, NULL, &rgb_cycle_data_##n,                      \
                            &rgb_cycle_config_##n, APPLICATION,                                    \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &rgb_cycle_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RGB_CYCLE_INST)

#endif

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
