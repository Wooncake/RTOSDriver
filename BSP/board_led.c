#include "LEDDriver.h"
#include "main.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} gpio_t;


static void Board_LED_WriteLevel(void *context, uint8_t level)
{
    gpio_t *gpio = (gpio_t *)context;

    HAL_GPIO_WritePin(
        gpio->port,
        gpio->pin,
        level ? GPIO_PIN_SET : GPIO_PIN_RESET
    );
}

//注册IO操作接口
static gpio_t led1_gpio =
{
    .port = Blue_LED_GPIO_Port,
    .pin  = Blue_LED_Pin
};

static gpio_t led2_gpio =
{
    .port = RED_LED_GPIO_Port,
    .pin  = RED_LED_Pin
};

static gpio_t led3_gpio =
{
    .port = Green_LED_GPIO_Port,
    .pin  = Green_LED_Pin
};


static led_io_t led1_io =
{
    .write_level = Board_LED_WriteLevel,
    .context = &led1_gpio
};

static led_io_t led2_io =
{
    .write_level = Board_LED_WriteLevel,
    .context = &led2_gpio
};

static led_io_t led3_io =
{
    .write_level = Board_LED_WriteLevel,
    .context = &led3_gpio
};

led_handle_t g_led1;
led_handle_t g_led2;
led_handle_t g_led3;

void Board_LED_Init(void)
{
    Led_Init(&g_led1, &led1_io, 0U);
    Led_Init(&g_led2, &led2_io, 0U);
    Led_Init(&g_led3, &led3_io, 0U);
}



