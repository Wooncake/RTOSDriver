#include "board_buzzer.h"
#include "tim.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} buzzer_gpio_t;

typedef struct
{
    buzzer_gpio_t gpio;
    uint32_t timer_tick_hz;
    volatile uint8_t running;
    volatile uint8_t output_level;
    volatile uint32_t high_ticks;
    volatile uint32_t low_ticks;
} buzzer_bsp_t;

static buzzer_bsp_t buzzer_bsp =
{
    {Buzzer_GPIO_Port, Buzzer_Pin},
    .timer_tick_hz = 1000000U,
    .running = 0U,
    .output_level = 0U,
    .high_ticks = 1U,
    .low_ticks = 1U
};

static void Buzzer_WriteLevel(uint8_t level)
{
    HAL_GPIO_WritePin(buzzer_bsp.gpio.port, buzzer_bsp.gpio.pin,
                      level != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void Buzzer_SetLevel(void *context, uint8_t level)
{
    buzzer_bsp_t *buzzer = (buzzer_bsp_t *)context;

    if (buzzer != 0)
    {
        HAL_GPIO_WritePin(buzzer->gpio.port, buzzer->gpio.pin,
                          level != 0U ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}


static buzzer_status_t Buzzer_StartTone(void *context, uint32_t frequency_hz, uint8_t duty_percent)
{
    buzzer_bsp_t *buzzer = (buzzer_bsp_t *)context;
    uint32_t period_ticks;
    uint32_t high_ticks;
    uint32_t low_ticks;

    if (buzzer == 0 || frequency_hz == 0U ||
        duty_percent == 0U || duty_percent >= 100U ||
        buzzer->timer_tick_hz < frequency_hz)
    {
        return BUZZER_INVALID_PARAM;
    }

    period_ticks = buzzer->timer_tick_hz / frequency_hz;
    if (period_ticks < 2U)
    {
        return BUZZER_INVALID_PARAM;
    }

    high_ticks = (period_ticks * duty_percent) / 100U;
    low_ticks = period_ticks - high_ticks;
    if (high_ticks == 0U || low_ticks == 0U)
    {
        return BUZZER_INVALID_PARAM;
    }

    buzzer->running = 0U;
    (void)HAL_TIM_Base_Stop_IT(&htim6);

    buzzer->high_ticks = high_ticks;
    buzzer->low_ticks = low_ticks;
    buzzer->output_level = 0U;
    Buzzer_WriteLevel(0U);

    __HAL_TIM_SET_AUTORELOAD(&htim6, low_ticks - 1U);
    __HAL_TIM_SET_COUNTER(&htim6, 0U);
    __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE);
    buzzer->running = 1U;

    if (HAL_TIM_Base_Start_IT(&htim6) != HAL_OK)
    {
        buzzer->running = 0U;
        Buzzer_WriteLevel(0U);
        return BUZZER_ERROR;
    }

    return BUZZER_OK;
}

static void Buzzer_StopTone(void *context)
{
    buzzer_bsp_t *buzzer = (buzzer_bsp_t *)context;

    if (buzzer != 0)
    {
        buzzer->running = 0U;
        (void)HAL_TIM_Base_Stop_IT(&htim6);
        buzzer->output_level = 0U;
        Buzzer_WriteLevel(0U);
    }
}

buzzer_handle_t g_buzzer;

void BSP_Buzzer_Init(void)
{
    buzzer_config_t config;

    config.type = BSP_BUZZER_TYPE;
    config.io.set_level = Buzzer_SetLevel;
    config.io.start_tone = Buzzer_StartTone;
    config.io.stop_tone = Buzzer_StopTone;
    config.io.context = &buzzer_bsp;
    config.active_level = 1U;
    config.default_duty_percent = 50U;

    Buzzer_WriteLevel(0U);
    (void)Buzzer_Init(&g_buzzer, &config);
}

void BSP_Buzzer_TimerUpdate(TIM_HandleTypeDef *timer)
{
    uint32_t next_ticks;

    if (timer == 0 || timer->Instance != TIM6 || buzzer_bsp.running == 0U)
    {
        return;
    }

    if (buzzer_bsp.output_level == 0U)
    {
        buzzer_bsp.output_level = 1U;
        Buzzer_WriteLevel(1U);
        next_ticks = buzzer_bsp.high_ticks;
    }
    else
    {
        buzzer_bsp.output_level = 0U;
        Buzzer_WriteLevel(0U);
        next_ticks = buzzer_bsp.low_ticks;
    }

    if (next_ticks > 0U)
    {
        __HAL_TIM_SET_AUTORELOAD(timer, next_ticks - 1U);
    }
}
