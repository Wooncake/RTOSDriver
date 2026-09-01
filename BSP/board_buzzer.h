#ifndef BOARD_BUZZER_H
#define BOARD_BUZZER_H

#include "BuzzerDriver.h"

/* Select the hardware connected to Buzzer_Pin. */
#ifndef BSP_BUZZER_TYPE
#define BSP_BUZZER_TYPE BUZZER_TYPE_PASSIVE
#endif

void BSP_Buzzer_Init(void);
void BSP_Buzzer_TimerUpdate(TIM_HandleTypeDef *timer);

#endif
