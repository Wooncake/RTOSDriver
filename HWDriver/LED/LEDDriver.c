#include "LEDDriver.h"

/* 初始化LED驱动
 * @param handle: LED句柄
 * @param io: LED的IO操作接口
 * @param active_level: 物理有效电平，1表示高电平点亮，0表示低电平点亮
 * @return 驱动状态
 * @example 只抽象逻辑，IO需要用户根据实际板卡适配
 * 
 */

drv_status_t Led_Init(led_handle_t *handle, led_io_t *io, uint8_t active_level)
{
    if (handle == NULL || io == NULL || io->write_level == NULL)
    {
        return DRV_INVALID_PARAM;
    }
    
    if(active_level != 0 && active_level != 1)
    {
        return DRV_INVALID_PARAM;
    }

    handle->io = *io;
    handle->active_level = active_level;
    handle->initialized = 1U;
    handle->is_on = 0U;

    handle->io.write_level(handle->io.context, !handle->active_level); // 初始化为关闭状态

    return DRV_OK;
}

drv_status_t LED_Set(led_handle_t *handle, bool on)
{
    uint8_t physical_level;
    if (handle == 0)
    {
        return DRV_INVALID_PARAM;
    }

    if (!handle->initialized)
    {
        return DRV_NOT_INIT;
    }

    physical_level = on ? handle->active_level : (uint8_t)!handle->active_level;
    handle->io.write_level(handle->io.context, physical_level);
    handle->is_on = on ? 1U : 0U;

    return DRV_OK;
}

drv_status_t LED_On(led_handle_t *handle)
{
    return LED_Set(handle, true);
}

drv_status_t LED_Off(led_handle_t *handle)
{
    return LED_Set(handle, false);
}


drv_status_t LED_Toggle(led_handle_t *handle)
{
    if (handle == 0)
    {
        return DRV_INVALID_PARAM;
    }

    return LED_Set(handle, handle->is_on == 0U);
}


bool LED_IsOn(const led_handle_t *handle)
{
    if (handle == 0 || !handle->initialized)
    {
        return false;
    }

    return handle->is_on != 0U;
}

