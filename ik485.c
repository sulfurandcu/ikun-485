/*
 * Copyright (c) 2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date             Author              Notes
 * 2024-12-08       liujitong           first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <ik485.h>

#define DBG_TAG     "ik485"
#define DBG_LVL     DBG_INFO
#include <rtdbg.h>

static void ik485_set_tx_on(ik485_device_t *ik485)
{
    RT_ASSERT(ik485 != RT_NULL);

    if (ik485->tx_pin != PIN_NONE)
    {
        if (ik485->tx_level == 1)
            rt_pin_write(ik485->tx_pin, PIN_HIGH);
        else
            rt_pin_write(ik485->tx_pin, PIN_LOW);

        if (ik485->tx_delay)
            rt_hw_us_delay(ik485->tx_delay);
    }
}

static void ik485_set_tx_off(ik485_device_t *ik485)
{
    RT_ASSERT(ik485 != RT_NULL);

    if (ik485->tx_pin != PIN_NONE)
    {
        if (ik485->tx_level == 1)
            rt_pin_write(ik485->tx_pin, PIN_LOW);
        else
            rt_pin_write(ik485->tx_pin, PIN_HIGH);
    }
}

static rt_err_t ik485_init(struct rt_device *dev)
{
    RT_ASSERT(dev != RT_NULL);
    ik485_device_t *ik485 = (ik485_device_t *)dev;

    if (ik485->tx_pin != PIN_NONE)
    {
        rt_pin_mode(ik485->tx_pin, PIN_MODE_OUTPUT);
    }

    return RT_EOK;
}

static rt_err_t ik485_open(struct rt_device *dev, rt_uint16_t oflag)
{
    RT_ASSERT(dev != RT_NULL);
    ik485_device_t *ik485 = (ik485_device_t *)dev;

    if (rt_device_open(ik485->serial, oflag) != RT_EOK)
    {
        LOG_E("ik485: serial open fail.");
        return -RT_ERROR;
    }

    if (ik485->tx_pin != PIN_NONE)
    {
        rt_pin_mode(ik485->tx_pin, PIN_MODE_OUTPUT);
    }

    return RT_EOK;
}

static rt_err_t ik485_close(struct rt_device *dev)
{
    RT_ASSERT(dev != RT_NULL);
    ik485_device_t *ik485 = (ik485_device_t *)dev;

    ik485->rx_notify.dev = RT_NULL;
    ik485->rx_notify.notify = RT_NULL;

    rt_device_close(ik485->serial);

    if (ik485->tx_pin != PIN_NONE)
    {
        rt_pin_mode(ik485->tx_pin, PIN_MODE_INPUT);
    }

    return RT_EOK;
}

static rt_ssize_t ik485_read(struct rt_device *dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    RT_ASSERT(dev != RT_NULL && (buffer != RT_NULL));
    if (size == 0) return 0;

    ik485_device_t *ik485 = (ik485_device_t *)dev;

    return rt_device_read(ik485->serial, pos, buffer, size);
}

static rt_ssize_t ik485_write(struct rt_device *dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    RT_ASSERT(dev != RT_NULL && (buffer != RT_NULL));
    if (size == 0) return 0;

    ik485_device_t *ik485 = (ik485_device_t *)dev;

    ik485_set_tx_on(ik485);
    rt_ssize_t len = rt_device_write(ik485->serial, pos, buffer, size);
    ik485_set_tx_off(ik485);

    return len;
}

static rt_err_t ik485_control(struct rt_device *dev, int cmd, void *args)
{
    RT_ASSERT(dev != RT_NULL);
    ik485_device_t *ik485 = (ik485_device_t *)dev;

    return rt_device_control(ik485->serial, cmd, args);
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops ik485_ops =
{
    ik485_init,
    ik485_open,
    ik485_close,
    ik485_read,
    ik485_write,
    ik485_control
};
#endif

static rt_err_t ik485_register(ik485_device_t *ik485, const char *name, rt_uint32_t flag, void *data)
{
    RT_ASSERT(ik485 != RT_NULL);

    rt_device_t device = &(ik485->parent);

    device->type        = RT_Device_Class_Char;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;

#ifdef RT_USING_DEVICE_OPS
    device->ops         = &ik485_ops;
#else
    device->init        = ik485_init;
    device->open        = ik485_open;
    device->close       = ik485_close;
    device->read        = ik485_read;
    device->write       = ik485_write;
    device->control     = ik485_control;
#endif
    device->user_data   = data;

    return rt_device_register(device, name, flag);
}

static void ik485_notify(rt_device_t dev)
{
    RT_ASSERT(dev != RT_NULL);

    ik485_device_t *ik485 = (ik485_device_t *)dev;

    if (ik485->parent.rx_indicate)
    {
        ik485->parent.rx_indicate(&ik485->parent, 0);
    }
}

rt_err_t ik485_create(const char *ik485_name, const char *serial_name,
                      rt_base_t tx_pin, rt_int32_t tx_level, rt_int32_t tx_delay)
{
    RT_ASSERT(ik485_name != RT_NULL);
    RT_ASSERT(serial_name != RT_NULL);

    rt_device_t serial = rt_device_find(serial_name);
    if (serial == RT_NULL)
    {
        LOG_E("ik485: create error, serial device(%s) not found.", serial_name);
        return RT_NULL;
    }

    ik485_device_t *ik485 = rt_malloc(sizeof(ik485_device_t));
    if (ik485 == RT_NULL)
    {
        LOG_E("ik485: create error, malloc fail.");
        return RT_NULL;
    }

    ik485->serial = serial;
    ik485->tx_pin = tx_pin;
    ik485->tx_level = tx_level;
    ik485->tx_delay = tx_delay;

    ik485_register(ik485, ik485_name, RT_DEVICE_FLAG_RDWR, RT_NULL);

    struct rt_device_notify notify;
    notify.notify = ik485_notify;
    notify.dev = &ik485->parent;
    rt_device_control(ik485->serial, RT_DEVICE_CTRL_NOTIFY_SET, &notify);

    LOG_D("ik485: create success: 0x%08x", ik485);

    return RT_EOK;
}

rt_err_t ik485_delete(ik485_device_t *ik485)
{
    if (ik485 == RT_NULL)
    {
        LOG_E("ik485: delete error, ik485 not exist.");
        return -RT_ERROR;
    }

    rt_free(ik485);

    LOG_D("ik485: delete success.");

    return RT_EOK;
}
