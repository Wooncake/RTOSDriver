#include "BuzzerDriver.h"

static uint8_t Buzzer_GetInactiveLevel(uint8_t active_level)
{
    return active_level ? 0U : 1U;
}

buzzer_status_t Buzzer_Init(buzzer_handle_t *handle, const buzzer_config_t *config)
{
    if (handle == 0 || config == 0)
    {
        return BUZZER_INVALID_PARAM;
    }

    if (config->type == BUZZER_TYPE_ACTIVE)
    {
        /*
         * 有源蜂鸣器必须提供 GPIO 电平控制函数。
         */
        if (config->io.set_level == 0)
        {
            return BUZZER_INVALID_PARAM;
        }

        if (config->active_level > 1U)
        {
            return BUZZER_INVALID_PARAM;
        }
    }
    else if (config->type == BUZZER_TYPE_PASSIVE)
    {
        /*
         * 无源蜂鸣器必须提供 PWM 启停函数。
         */
        if (config->io.start_tone == 0 || config->io.stop_tone == 0)
        {
            return BUZZER_INVALID_PARAM;
        }

        if (config->default_duty_percent == 0U || config->default_duty_percent > 100U)
        {
            return BUZZER_INVALID_PARAM;
        }
    }
    else
    {
        return BUZZER_INVALID_PARAM;
    }

    handle->config = *config;
    handle->active = 0U;
    handle->start_tick = 0U;
    handle->duration_ms = 0U;
    handle->frequency_hz = 0U;
    handle->initialized = 1U;

    /*
     * 初始化时强制关闭，防止上电误响。
     */
    Buzzer_Stop(handle);

    return BUZZER_OK;
}

buzzer_status_t Buzzer_Start(buzzer_handle_t *handle, uint32_t frequency_hz, uint32_t duration_ms, uint32_t now_ms)
{
    buzzer_status_t status;

    if (handle == 0)
    {
        return BUZZER_INVALID_PARAM;
    }

    if(!handle->initialized)
    {
        return BUZZER_NOT_INIT;
    }

    if (handle->config.type == BUZZER_TYPE_ACTIVE) //有源模式
    {
        /*
         * 有源蜂鸣器不需要频率。
         */
        handle->config.io.set_level(handle->config.io.context, handle->config.active_level);

        handle->frequency_hz = 0U;
    }
    else
    {
        /*
         * 无源蜂鸣器必须指定有效频率。
         */
        if (frequency_hz == 0U || handle->config.io.start_tone == 0)
        {
            return BUZZER_INVALID_PARAM;
        }

        status = handle->config.io.start_tone(handle->config.io.context, frequency_hz, handle->config.default_duty_percent);

        if (status != BUZZER_OK)
        {
            return status;
        }

        handle->frequency_hz = frequency_hz;
    }

    handle->start_tick = now_ms;
    handle->duration_ms = duration_ms;
    handle->active = 1U;

    return BUZZER_OK;


}


buzzer_status_t Buzzer_Stop(buzzer_handle_t *handle)
{
    if (handle == 0)
    {
        return BUZZER_INVALID_PARAM;
    }

    if (!handle->initialized)
    {
        return BUZZER_NOT_INIT;
    }

    if (handle->config.type == BUZZER_TYPE_ACTIVE)
    {
        if (handle->config.io.set_level != 0)
        {
            handle->config.io.set_level(handle->config.io.context, Buzzer_GetInactiveLevel(handle->config.active_level));
        }
    }
    else
    {
        if (handle->config.io.stop_tone != 0)
        {
            handle->config.io.stop_tone(handle->config.io.context);
        }
    }

    handle->active = 0U;
    handle->frequency_hz = 0U;
    handle->duration_ms = 0U;

    return BUZZER_OK;
}



void Buzzer_Update(buzzer_handle_t *handle, uint32_t now_ms)
{
    if (handle == 0 || !handle->initialized || !handle->active)
    {
        return;
    }

    /*
     * duration_ms == 0 表示持续输出。
     */
    if (handle->duration_ms == 0U)
    {
        return;
    }

    /*
     * 使用无符号减法，支持 tick 溢出。
     */
    if ((uint32_t)(now_ms - handle->start_tick) >= handle->duration_ms)
    {
        Buzzer_Stop(handle);
    }
}

bool Buzzer_IsActive(const buzzer_handle_t *handle)
{
    if (handle == 0 || !handle->initialized)
    {
        return false;
    }

    return handle->active != 0U;
}





