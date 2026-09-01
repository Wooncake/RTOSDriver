#include "LedAPP.h"

extern led_handle_t g_led1;
extern led_handle_t g_led2;
extern led_handle_t g_led3;
extern button_handle_t g_key1;
extern buzzer_handle_t g_buzzer;

void App_Test(void)
{
    uint32_t events;
    Buzzer_Update(&g_buzzer, HAL_GetTick());
    Button_Update(&g_key1, HAL_GetTick());

    events = Button_GetEvents(&g_key1);

    if (events & BUTTON_EVENT_CLICK)
    {
        LED_Toggle(&g_led1);
        (void)Buzzer_Start(&g_buzzer, 2000U, 80U, HAL_GetTick());
    }

    if (events & BUTTON_EVENT_DOUBLE)
    {
        LED_Toggle(&g_led2);
        (void)Buzzer_Start(&g_buzzer, 1800U, 500U, HAL_GetTick());
    }

    if (events & BUTTON_EVENT_LONG_START)
    {
        LED_Toggle(&g_led3);
        (void)Buzzer_Start(&g_buzzer, 1600U, 1000U, HAL_GetTick());
    }
    
}


