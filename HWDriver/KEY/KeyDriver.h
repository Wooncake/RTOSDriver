#ifndef _KEYDRIVER_H_
#define _KEYDRIVER_H_


#include "main.h"
#include <stdbool.h>

typedef enum
{
    BUTTON_OK = 0,
    BUTTON_ERROR,
    BUTTON_INVALID_PARAM
} button_status_t;

/* 按键事件*/
typedef enum
{
    BUTTON_EVENT_NONE       = 0U,
    BUTTON_EVENT_DOWN       = 1U << 0,
    BUTTON_EVENT_UP         = 1U << 1,
    BUTTON_EVENT_CLICK      = 1U << 2,
    BUTTON_EVENT_DOUBLE     = 1U << 3,
    BUTTON_EVENT_LONG_START = 1U << 4
} button_event_t;

/* 按键状态 */
typedef enum
{
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_DEBOUNCE_PRESS,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_DEBOUNCE_RELEASE,

    /* 第一次点击完成，等待第二次点击 */
    BUTTON_STATE_WAIT_SECOND_CLICK,
    /* 第二次按下消抖 */
    BUTTON_STATE_DEBOUNCE_SECOND_PRESS

} button_state_t;


typedef uint8_t (*button_read_level_fn)(void *context);

typedef struct
{
    button_read_level_fn read_level;    //读取按键电平的函数指针
    void *context;                      //上下文指针，传递给read_level函数，用于读取按键电平的具体实现
} button_io_t;

typedef struct
{
    button_io_t io;                     //按键的IO操作接口
    /*
     * 按下时的物理电平
     * 例如：
     * 低电平按下：active_level = 0
     * 高电平按下：active_level = 1
     */
    uint8_t active_level;               //物理有效电平，1表示高电平按下，0表示低电平按下

    uint32_t debounce_ms;               //消抖时间，单位毫秒
    uint32_t long_press_ms;             //长按时间，单位毫秒
    uint32_t click_gap_ms;              //点击间隔时间，单位毫秒，用于判断双击事件

} button_config_t;


/* 按键示例 */
typedef struct
{
    button_config_t config;

    button_state_t state;


    bool raw_pressed;       //原始按下状态，未经过消抖处理
    bool stable_pressed;    //稳定按下状态，经过消抖处理

    bool long_reported;     //长按事件是否已经上报过

     /*
     * 0：当前没有等待点击
     * 1：已经完成第一次点击，等待第二次点击
     */
    uint8_t click_count;    //点击次数

    uint32_t raw_change_tick; //原始状态变化的时间戳，单位毫秒
    uint32_t press_tick;    //按下的时间戳，单位毫秒
    uint32_t release_tick;  //释放的时间戳，单位毫秒
    uint32_t release_pre_tick; //上一次释放的时间戳，单位毫秒

    uint32_t pending_events;//待处理的按键事件

    bool initialized;       //是否已经初始化

} button_handle_t;


button_status_t Button_Init(button_handle_t *handle, const button_config_t *config);

void Button_Update(button_handle_t *handle, uint32_t now_ms);

uint32_t Button_GetEvents(button_handle_t *handle);

bool Button_IsPressed(const button_handle_t *handle);



#endif
