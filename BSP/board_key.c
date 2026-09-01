#include "KeyDriver.h"
#include "main.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} gpio_input_t;



static uint8_t Button_ReadLevel(void *context)
{
    gpio_input_t *gpio;

    gpio = (gpio_input_t *)context;

    return (uint8_t)HAL_GPIO_ReadPin(gpio->port, gpio->pin);
}

static gpio_input_t key1_gpio =
{
    .port = KEY1_GPIO_Port,
    .pin  = KEY1_Pin
};

button_handle_t g_key1;


void BSP_Button_Init(void)
{
    button_config_t config;

    config.io.read_level = Button_ReadLevel;
    config.io.context = &key1_gpio;

    /*
     * 假设按键按下时 GPIO 为低电平。
     */
    config.active_level = GPIO_PIN_RESET;

    config.debounce_ms = 20U;
    config.long_press_ms = 1000U;
    config.click_gap_ms = 250U;

    Button_Init(&g_key1, &config);
}


