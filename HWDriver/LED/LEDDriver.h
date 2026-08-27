#ifndef _LEDDRIVER_H_
#define _LEDDRIVER_H_

#include "stdio.h"
#include <stdint.h>
#include <stdbool.h>

/* 驱动状态枚举 */
typedef enum
{
    DRV_OK = 0,
    DRV_ERROR,
    DRV_INVALID_PARAM,
    DRV_NOT_INIT
} drv_status_t;


/* IO状态 */
typedef struct {
    
    void (*write_level)(void *context, uint8_t level);
    void *context;

}led_io_t;


/* LED句柄 */
typedef struct
{
    led_io_t io;

    /* 物理有效电平：1 表示高电平点亮，0 表示低电平点亮 */
    uint8_t active_level;

    uint8_t initialized;
    uint8_t is_on;

} led_handle_t;


drv_status_t Led_Init(led_handle_t *handle, led_io_t *io, uint8_t active_level);
drv_status_t Led_DeInit(led_handle_t *handle);

drv_status_t LED_Set(led_handle_t *handle, bool on);
drv_status_t LED_On(led_handle_t *handle);
drv_status_t LED_Off(led_handle_t *handle);

drv_status_t LED_Toggle(led_handle_t *handle);
bool LED_IsOn(const led_handle_t *handle);

#endif




