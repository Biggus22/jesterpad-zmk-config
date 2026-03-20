// Copyright (c) 2026
// SPDX-License-Identifier: MIT

#include <zephyr/init.h>

#include <zmk/endpoints.h>

static int jesterpad_force_usb_output(void) {
    return zmk_endpoints_select_transport(ZMK_TRANSPORT_USB);
}

SYS_INIT(jesterpad_force_usb_output, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
