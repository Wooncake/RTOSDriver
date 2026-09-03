#ifndef _BUZZERDRIVER_H_
#define _BUZZERDRIVER_H_

#include "main.h"
#include <stdbool.h>

/* 有源蜂鸣器 以及 无源蜂鸣器 */
typedef enum
{
    BUZZER_TYPE_ACTIVE = 0,
    BUZZER_TYPE_PASSIVE

} buzzer_type_t; 


typedef enum
{
    BUZZER_OK = 0,
    BUZZER_ERROR,
    BUZZER_INVALID_PARAM,
    BUZZER_NOT_INIT,
    BUZZER_BUSY

} buzzer_status_t;

typedef enum
{
    BUZZER_SOUND_SHORT = 0,
    BUZZER_SOUND_CONFIRM,
    BUZZER_SOUND_ALARM
} buzzer_sound_t;

typedef struct
{
    uint32_t frequency_hz;
    uint32_t duration_ms;
} buzzer_segment_t;

typedef void (*buzzer_set_level_fn)(void *context, uint8_t level);

typedef buzzer_status_t (*buzzer_start_tone_fn)(void *context, uint32_t frequency_hz, uint8_t duty_percent);

typedef void (*buzzer_stop_tone_fn)(void *context);

typedef struct
{
    /*
     * 有源蜂鸣器使用。
     */
    buzzer_set_level_fn set_level;

    /*
     * 无源蜂鸣器使用。
     */
    buzzer_start_tone_fn start_tone;
    buzzer_stop_tone_fn stop_tone;

    /*
     * 指向 GPIO、定时器等硬件资源。
     */
    void *context;

} buzzer_io_t;


typedef struct
{
    buzzer_type_t type;
    buzzer_io_t io;
    /*
     * 有源蜂鸣器的有效电平。
     *
     * 高电平有效：1
     * 低电平有效：0
     */
    uint8_t active_level;

    /*
     * 无源蜂鸣器默认占空比。
     * 通常设置为 50。
     */
    uint8_t default_duty_percent;

} buzzer_config_t;



typedef struct
{
    buzzer_config_t config;

    uint8_t active;

    uint32_t start_tick;
    uint32_t duration_ms;
    uint32_t frequency_hz;

    const buzzer_segment_t *pattern;
    uint8_t pattern_length;
    uint8_t pattern_index;

    uint8_t initialized;


} buzzer_handle_t;

buzzer_status_t Buzzer_Init(buzzer_handle_t *handle, const buzzer_config_t *config);

buzzer_status_t Buzzer_Start(buzzer_handle_t *handle, uint32_t frequency_hz, uint32_t duration_ms, uint32_t now_ms);

buzzer_status_t Buzzer_PlaySound(buzzer_handle_t *handle, buzzer_sound_t sound, uint32_t now_ms);

buzzer_status_t Buzzer_Stop(buzzer_handle_t *handle);

void Buzzer_Update(buzzer_handle_t *handle, uint32_t now_ms);

bool Buzzer_IsActive(const buzzer_handle_t *handle);

#endif



