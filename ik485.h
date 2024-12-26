/**
 * Copyright (c) 2024, sulfurandcu.github.io
 *
 * SPDX-License-Identifier: MIT
 *
 * Change Logs:
 * Date             Author              Notes
 * 2024-12-08       liujitong           first version
 */

#ifndef __ikun_485_h__
#define __ikun_485_h__

#include <rtthread.h>

typedef struct ik485_device
{
    struct rt_device  parent;
    struct rt_device *serial;

    rt_base_t   tx_pin;
    rt_uint32_t tx_level;
    rt_uint32_t tx_delay;

    struct rt_device_notify rx_notify;

} ik485_device_t;

// tx_delay: typical value is 10us.
ik485_device_t *ik485_create(const char *ik485_name, const char *serial_name, rt_base_t tx_pin, rt_int32_t tx_level, rt_int32_t tx_delay);
rt_err_t ik485_delete(ik485_device_t *ik485);

#endif
