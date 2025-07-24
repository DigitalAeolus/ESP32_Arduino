#ifndef LV_WINDCHIME_SIMPLE_H
#define LV_WINDCHIME_SIMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>

extern lv_obj_t * windchime_simple_screen;

void WindChimeSimpleCreate(lv_event_cb_t event_cb);
void WindChimeSimpleAddEvent(const char* source, int intensity);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*LV_WINDCHIME_SIMPLE_H*/