// Copyright (c) 2026
// SPDX-License-Identifier: MIT

#define DT_DRV_COMPAT zmk_behavior_rgb_preset

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <drivers/behavior.h>

#include <zmk/behavior.h>
#include <zmk/rgb_underglow.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT)

struct rgb_preset_config {
    uint16_t h;
    uint8_t s;
    uint8_t b;
    uint8_t effect;
    uint8_t speed;
};

#if IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)

#define RGB_SPEED_MIN 1
#define RGB_SPEED_MAX 5

static int apply_speed(uint8_t target) {
    target = CLAMP(target, RGB_SPEED_MIN, RGB_SPEED_MAX);

    // Normalize to minimum speed to get deterministic wrap-around behaviour.
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

#endif // CONFIG_ZMK_RGB_UNDERGLOW

static int rgb_preset_pressed(struct zmk_behavior_binding *binding,
                              struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);

#if !IS_ENABLED(CONFIG_ZMK_RGB_UNDERGLOW)
    LOG_WRN("RGB preset invoked without underglow enabled");
    return -ENOTSUP;
#else
    const struct device *dev = zmk_behavior_get_binding(binding->behavior_dev);
    const struct rgb_preset_config *cfg = dev->config;

    int rc = zmk_rgb_underglow_on();
    if (rc < 0) {
        LOG_ERR("RGB underglow on failed: %d", rc);
        return rc;
    }

    rc = zmk_rgb_underglow_select_effect(cfg->effect);
    if (rc < 0) {
        LOG_ERR("RGB select effect %u failed: %d", cfg->effect, rc);
        return rc;
    }

    struct zmk_led_hsb color = {
        .h = cfg->h,
        .s = cfg->s,
        .b = cfg->b,
    };

    rc = zmk_rgb_underglow_set_hsb(color);
    if (rc < 0) {
        LOG_ERR("RGB set color failed: %d", rc);
        return rc;
    }

    rc = apply_speed(cfg->speed);
    if (rc < 0) {
        LOG_ERR("RGB set speed failed: %d", rc);
        return rc;
    }

    return 0;
#endif
}

static int rgb_preset_released(struct zmk_behavior_binding *binding,
                               struct zmk_behavior_binding_event event) {
    ARG_UNUSED(binding);
    ARG_UNUSED(event);
    return ZMK_BEHAVIOR_OPAQUE;
}

static const struct behavior_driver_api rgb_preset_driver_api = {
    .binding_pressed = rgb_preset_pressed,
    .binding_released = rgb_preset_released,
};

#define RGB_PRESET_INST(n)                                                                         \
    static const struct rgb_preset_config rgb_preset_config_##n = {                                \
        .h = DT_INST_PROP_OR(n, h, 0),                                                              \
        .s = DT_INST_PROP_OR(n, s, 100),                                                            \
        .b = DT_INST_PROP_OR(n, b, 100),                                                            \
        .effect = DT_INST_PROP_OR(n, effect, 0),                                                    \
        .speed = DT_INST_PROP_OR(n, speed, 3),                                                      \
    };                                                                                             \
    BEHAVIOR_DT_INST_DEFINE(n, NULL, NULL, NULL, &rgb_preset_config_##n, APPLICATION,              \
                            CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &rgb_preset_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RGB_PRESET_INST)

#endif /* DT_HAS_COMPAT_STATUS_OKAY(DT_DRV_COMPAT) */
