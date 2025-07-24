#ifndef CONTROL_PANEL_H
#define CONTROL_PANEL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <lvgl.h>

extern lv_obj_t * control_panel_screen;

void ControlPanelCreate(lv_event_cb_t event_cb);
void ControlPanelShow(void);
void ControlPanelHide(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*CONTROL_PANEL_H*/