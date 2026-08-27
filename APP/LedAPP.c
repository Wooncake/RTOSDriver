#include "LedAPP.h"

extern led_handle_t g_led1;
extern led_handle_t g_led2;
extern led_handle_t g_led3;
extern button_handle_t g_key1;

void App_Test(void)
{
    uint32_t events;

    Button_Update(&g_key1, HAL_GetTick());

    events = Button_GetEvents(&g_key1);

    if (events & BUTTON_EVENT_CLICK)
    {
        LED_Toggle(&g_led1);
    }

    if (events & BUTTON_EVENT_DOUBLE)
    {
        LED_Toggle(&g_led2);
    }

    if (events & BUTTON_EVENT_LONG_START)
    {
        LED_Toggle(&g_led3);
    }
    
}


