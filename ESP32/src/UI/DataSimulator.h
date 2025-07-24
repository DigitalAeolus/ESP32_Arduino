#ifndef DATA_SIMULATOR_H
#define DATA_SIMULATOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>
#include "WindChime.h"

// 数据模拟器函数
void DataSimulatorInit(void);
void DataSimulatorStart(void);
void DataSimulatorStop(void);

// 手动触发事件（用于测试）
void SimulateGitHubEvent(void);
void SimulateWikipediaEvent(void);
void SimulateWeatherUpdate(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*DATA_SIMULATOR_H*/