#include "KeyDriver.h"


static bool Button_Elapsed(uint32_t now, uint32_t start, uint32_t period)
{
    return ((uint32_t)(now - start) >= period);
}

button_status_t Button_Init(button_handle_t *handle, const button_config_t *config)
{
    bool pressed;

    if(handle == NULL || config == NULL || config->io.read_level == NULL)
    {
        return BUTTON_INVALID_PARAM;
    }

    handle->config = *config;
    
    pressed = (config->io.read_level(config->io.context) == config->active_level);

    handle->raw_pressed = pressed;
    handle->stable_pressed = pressed;

    if (pressed)
    {
        handle->state = BUTTON_STATE_PRESSED;
        handle->press_tick = 0U;
    }
    else
    {
        handle->state = BUTTON_STATE_RELEASED;
    }
    handle->long_reported = false;
    handle->click_count = 0;
    handle->raw_change_tick = 0;
    handle->press_tick = 0;
    handle->release_tick = 0;
    handle->pending_events = BUTTON_EVENT_NONE;
    handle->initialized = true;

    return BUTTON_OK;
}


void Button_Update(button_handle_t *handle, uint32_t now_ms)
{
    bool current_pressed;

    if (handle == 0 || !handle->initialized)
    {
        return;
    }

    current_pressed =
        (handle->config.io.read_level(handle->config.io.context)
         == handle->config.active_level);

    switch (handle->state)
    {
        case BUTTON_STATE_RELEASED:
        {
            if (current_pressed)
            {
                handle->raw_change_tick = now_ms;
                handle->state = BUTTON_STATE_DEBOUNCE_PRESS;
            }

            break;
        }

        case BUTTON_STATE_DEBOUNCE_PRESS:
        {
            /*
             * 按下信号消失，说明可能是抖动。
             */
            if (!current_pressed)
            {
                handle->state = BUTTON_STATE_RELEASED;
            }
            else if (Button_Elapsed(now_ms,
                                    handle->raw_change_tick,
                                    handle->config.debounce_ms))
            {
                /*
                 * 确认第一次按下。
                 */
                handle->raw_pressed = true;
                handle->stable_pressed = true;
                handle->press_tick = now_ms;
                handle->long_reported = false;

                handle->pending_events |= BUTTON_EVENT_DOWN;
                handle->state = BUTTON_STATE_PRESSED;
            }

            break;
        }

        case BUTTON_STATE_PRESSED:
        {
            /*
             * 检测释放，先进入释放消抖。
             */
            if (!current_pressed)
            {
                handle->raw_change_tick = now_ms;
                handle->state = BUTTON_STATE_DEBOUNCE_RELEASE;
            }
            else if (!handle->long_reported &&
                     Button_Elapsed(now_ms,
                                    handle->press_tick,
                                    handle->config.long_press_ms))
            {
                /*
                 * 长按只上报一次。
                 */
                handle->long_reported = true;
                handle->pending_events |= BUTTON_EVENT_LONG_START;
            }

            break;
        }

        case BUTTON_STATE_DEBOUNCE_RELEASE:
        {
            /*
             * 释放过程中又恢复成按下，说明是抖动。
             */
            if (current_pressed)
            {
                handle->state = BUTTON_STATE_PRESSED;
            }
            else if (Button_Elapsed(now_ms,
                                    handle->raw_change_tick,
                                    handle->config.debounce_ms))
            {
                /*
                 * 确认释放。
                 */
                handle->raw_pressed = false;
                handle->stable_pressed = false;

                handle->pending_events |= BUTTON_EVENT_UP;

                if (handle->long_reported)
                {
                    /*
                     * 长按结束不算单击。
                     */
                    handle->click_count = 0U;
                    handle->state = BUTTON_STATE_RELEASED;
                }
                else if (handle->click_count == 0U)
                {
                    /*
                     * 第一次点击完成。
                     *
                     * 此时不能立即产生 CLICK，
                     * 因为还要等待第二次点击。
                     */
                    handle->click_count = 1U;
                    handle->release_tick = now_ms;
                    handle->state = BUTTON_STATE_WAIT_SECOND_CLICK;
                }
                else
                {
                    /*
                     * 第二次点击释放，确认双击。
                     */
                    handle->pending_events |= BUTTON_EVENT_DOUBLE;

                    handle->click_count = 0U;
                    handle->state = BUTTON_STATE_RELEASED;
                }
            }

            break;
        }

        case BUTTON_STATE_WAIT_SECOND_CLICK:
        {
            /*
             * 在等待第二次点击期间检测到按下。
             */
            if (current_pressed)
            {
                handle->raw_change_tick = now_ms;
                handle->state = BUTTON_STATE_DEBOUNCE_SECOND_PRESS;
            }
            else if (Button_Elapsed(now_ms,
                                   handle->release_tick,
                                   handle->config.click_gap_ms))
            {
                /*
                 * 等待窗口结束，没有第二次点击，
                 * 因此第一次点击确认是单击。
                 */
                handle->pending_events |= BUTTON_EVENT_CLICK;

                handle->click_count = 0U;
                handle->state = BUTTON_STATE_RELEASED;
            }

            break;
        }

        case BUTTON_STATE_DEBOUNCE_SECOND_PRESS:
        {
            /*
             * 第二次按下过程中恢复为释放，
             * 说明可能是抖动，继续等待第二次点击。
             */
            if (!current_pressed)
            {
                handle->state = BUTTON_STATE_WAIT_SECOND_CLICK;
            }
            else if (Button_Elapsed(now_ms,
                                    handle->raw_change_tick,
                                    handle->config.debounce_ms))
            {
                /*
                 * 确认第二次按下。
                 */
                handle->raw_pressed = true;
                handle->stable_pressed = true;
                handle->press_tick = now_ms;
                handle->long_reported = false;

                handle->pending_events |= BUTTON_EVENT_DOWN;
                handle->state = BUTTON_STATE_PRESSED;
            }

            break;
        }

        default:
        {
            handle->state = BUTTON_STATE_RELEASED;
            handle->click_count = 0U;
            break;
        }
    }
}

uint32_t Button_GetEvents(button_handle_t *handle)
{
    uint32_t events = 0U;

    if(handle == NULL || !handle->initialized)
    {
        return 0U;
    }

    events = handle->pending_events;
    handle->pending_events = BUTTON_EVENT_NONE;

    return events;
}


bool Button_IsPressed(const button_handle_t *handle)
{
    if (handle == 0 || !handle->initialized)
    {
        return false;
    }

    return handle->stable_pressed;
}


