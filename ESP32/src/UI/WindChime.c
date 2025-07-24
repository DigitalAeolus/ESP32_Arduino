#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include "WindChime.h"
#include "WindChimeConfig.h"
#include "Screenbase.h"
#include "AudioFeedback.h"

// --- Configuration Constants ---
#define MAX_PARTICLES WINDCHIME_MAX_PARTICLES
#define MAX_RIPPLES WINDCHIME_MAX_RIPPLES
#define MAX_EVENTS_HISTORY 20
#define MAX_LOG_LINES 8
#define CANVAS_WIDTH 480
#define CANVAS_HEIGHT 480
#define CENTER_X (CANVAS_WIDTH / 2)
#define CENTER_Y (CANVAS_HEIGHT / 2)

// --- Global Variables ---
lv_obj_t * windchime_screen = NULL;
static lv_obj_t * main_canvas = NULL;
static lv_obj_t * center_orb = NULL;
static lv_obj_t * volume_bar = NULL;
static lv_obj_t * event_log_label = NULL;

// --- Animation & Effect Data ---
static particle_t particles[MAX_PARTICLES];
static ripple_t ripples[MAX_RIPPLES];
static wind_chime_event_t event_history[MAX_EVENTS_HISTORY];
static uint8_t event_count = 0;
static lv_timer_t * animation_timer = NULL;

// --- Environmental Parameters ---
static int16_t current_wind_speed = 0;
static int16_t current_temperature = 20;
static uint8_t audio_volume = 50;
static uint32_t last_event_time = 0;

// --- Color Theme ---
static const lv_color_t source_colors[DATA_SOURCE_MAX] = {
    LV_COLOR_MAKE(0x4A, 0x90, 0xE2),   // GitHub - Blue
    LV_COLOR_MAKE(0x7B, 0x68, 0xEE),   // Wikipedia - Purple
    LV_COLOR_MAKE(0x50, 0xC8, 0x78)    // Weather - Green
};
static const char* source_names[] = {"GitHub", "Wikipedia", "Weather"};

// --- Forward Declarations ---
static void animation_callback(lv_timer_t * timer);
static void create_ripple(int16_t x, int16_t y, lv_color_t color, uint16_t max_radius);
static void create_particles(int16_t x, int16_t y, lv_color_t color, uint8_t count);
static void update_particles(void);
static void update_ripples(void);
static void update_center_orb(void);
static void add_event_to_log(wind_chime_event_t* event);
static void create_visual_objects(void);
static void update_visual_objects(void);


// =================================================================
// --- Screen Creation ---
// =================================================================
void WindChimeScreenCreate(lv_event_cb_t event_cb)
{
    windchime_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(windchime_screen, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(windchime_screen, LV_OPA_COVER, LV_PART_MAIN);

    event_log_label = lv_label_create(windchime_screen);
    lv_obj_set_width(event_log_label, lv_pct(95));
    lv_obj_align(event_log_label, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_label_set_recolor(event_log_label, true);
    lv_obj_set_style_text_font(event_log_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(event_log_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_set_style_text_line_space(event_log_label, 4, 0);
    lv_label_set_text(event_log_label, "Awaiting events from the digital ether...");
    lv_obj_move_background(event_log_label);

    main_canvas = lv_canvas_create(windchime_screen);
    lv_obj_set_size(main_canvas, CANVAS_WIDTH, CANVAS_HEIGHT);
    lv_obj_align(main_canvas, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(main_canvas, LV_OPA_TRANSP, LV_PART_MAIN);

    lv_obj_t * settings_btn = lv_btn_create(windchime_screen);
    lv_obj_align(settings_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_add_event_cb(settings_btn, event_cb, LV_EVENT_CLICKED, (void *)1);
    lv_obj_set_style_bg_opa(settings_btn, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(settings_btn, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(settings_btn, 0, LV_PART_MAIN);
    
    lv_obj_t * icon = lv_label_create(settings_btn);
    lv_label_set_text(icon, LV_SYMBOL_SETTINGS);
    lv_obj_set_style_text_color(icon, lv_color_hex(0xCCCCCC), 0);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
    lv_obj_center(icon);

    center_orb = lv_obj_create(windchime_screen);
    lv_obj_set_size(center_orb, 80, 80);
    lv_obj_align(center_orb, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_radius(center_orb, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(center_orb, lv_color_make(0x2a, 0x2a, 0x2a), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(center_orb, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_border_width(center_orb, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(center_orb, lv_color_make(0x4a, 0x4a, 0x4a), LV_PART_MAIN);
    lv_obj_set_style_shadow_width(center_orb, 20, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(center_orb, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(center_orb, LV_OPA_50, LV_PART_MAIN);
    
    volume_bar = lv_bar_create(windchime_screen);
    lv_obj_set_size(volume_bar, 4, 60);
    lv_obj_align(volume_bar, LV_ALIGN_TOP_LEFT, 15, 15);
    lv_obj_set_style_bg_color(volume_bar, lv_color_make(0x2a, 0x2a, 0x2a), LV_PART_MAIN);
    lv_obj_set_style_bg_color(volume_bar, lv_color_make(0x4A, 0x90, 0xE2), LV_PART_INDICATOR);
    lv_bar_set_range(volume_bar, 0, 100);
    lv_bar_set_value(volume_bar, audio_volume, LV_ANIM_OFF);

    memset(particles, 0, sizeof(particles));
    memset(ripples, 0, sizeof(ripples));
    memset(event_history, 0, sizeof(event_history));
    
    create_visual_objects();
    
    WindChimeStartAnimation();
}


// =================================================================
// --- Event Handling ---
// =================================================================
void WindChimeAddEvent(wind_chime_event_t* event)
{
    if (!event) return;
    
    event_history[event_count % MAX_EVENTS_HISTORY] = *event;
    event_count++;
    
    int16_t x, y;
    lv_color_t color = source_colors[event->source];
    
    switch(event->source) {
        case DATA_SOURCE_GITHUB:
            x = CENTER_X + (rand() % 200) - 100;
            y = CENTER_Y + (rand() % 200) - 100;
            break;
        case DATA_SOURCE_WIKIPEDIA:
            x = CENTER_X + (rand() % 160) - 80;
            y = CENTER_Y + (rand() % 160) - 80;
            break;
        case DATA_SOURCE_WEATHER:
        default:
            x = CENTER_X;
            y = CENTER_Y;
            break;
    }
    
    uint16_t ripple_size = 50 + (event->intensity * 2);
    create_ripple(x, y, color, ripple_size);
    uint8_t particle_count = 3 + (event->intensity / 20);
    create_particles(x, y, color, particle_count);
    
    PlayEventSound(event->source, event->intensity);
    
    add_event_to_log(event);
    
    lv_obj_set_style_shadow_color(center_orb, color, LV_PART_MAIN);
    last_event_time = lv_tick_get();
}

// =================================================================
// --- Log & Animation Updates ---
// =================================================================

static void add_event_to_log(wind_chime_event_t* event) {
    static bool first_event = true;
    const char * current_log_text = lv_label_get_text(event_log_label);
    char new_log_buffer[1024];

    // FINAL FIX: Use lv_color_to_u32() and bitwise operations. This is a robust,
    // universal way to get R,G,B values in LVGL v9, avoiding build-specific issues.
    lv_color_t color = source_colors[event->source];
    uint32_t color32 = lv_color_to_u32(color);
    uint8_t r = (color32 >> 16) & 0xFF;
    uint8_t g = (color32 >> 8) & 0xFF;
    uint8_t b = color32 & 0xFF;

    char color_hex_str[7]; // RRGGBB
    sprintf(color_hex_str, "%02X%02X%02X", r, g, b);

    char new_line[256];
    snprintf(new_line, sizeof(new_line), "#%s>>[%s]# %s",
             color_hex_str,
             source_names[event->source],
             event->description);

    if (first_event) {
        lv_label_set_text(event_log_label, new_line);
        first_event = false;
        return;
    }

    int line_count = 1;
    for (const char* t = current_log_text; *t; t++) {
        if (*t == '\n') line_count++;
    }

    if (line_count < MAX_LOG_LINES) {
        snprintf(new_log_buffer, sizeof(new_log_buffer), "%s\n%s", current_log_text, new_line);
    } else {
        const char* start_of_second_line = strchr(current_log_text, '\n');
        if (start_of_second_line) {
            snprintf(new_log_buffer, sizeof(new_log_buffer), "%s\n%s", start_of_second_line + 1, new_line);
        } else {
            snprintf(new_log_buffer, sizeof(new_log_buffer), "%s", new_line);
        }
    }
    lv_label_set_text(event_log_label, new_log_buffer);
}

static void update_center_orb(void)
{
    uint32_t time_since_event = lv_tick_get() - last_event_time;
    
    if (time_since_event < 2000) {
        uint8_t brightness = LV_OPA_COVER - (time_since_event * LV_OPA_COVER / 2000);
        lv_obj_set_style_shadow_opa(center_orb, brightness, LV_PART_MAIN);
    } else {
        lv_obj_set_style_shadow_opa(center_orb, LV_OPA_20, LV_PART_MAIN);
    }
    
    int16_t shake_x = 0, shake_y = 0;
    if (current_wind_speed > 5) {
        int16_t shake_range = current_wind_speed / 5;
        if (shake_range < 1) shake_range = 1;
        shake_x = (rand() % (shake_range * 2)) - shake_range;
        shake_y = (rand() % (shake_range * 2)) - shake_range;
    }
    lv_obj_align(center_orb, LV_ALIGN_CENTER, shake_x, shake_y);
}


// =================================================================
// --- Other Helper Functions (Unchanged) ---
// =================================================================

void WindChimeUpdateWeather(int16_t wind_speed, int16_t temperature) {
    current_wind_speed = wind_speed;
    current_temperature = temperature;
    UpdateAmbientSound(wind_speed);
}

void WindChimeSetVolume(uint8_t volume) {
    audio_volume = volume;
    lv_bar_set_value(volume_bar, volume, LV_ANIM_ON);
    SetAudioVolume(volume);
}

static void create_ripple(int16_t x, int16_t y, lv_color_t color, uint16_t max_radius) {
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (!ripples[i].active) {
            ripples[i] = (ripple_t){.active = true, .x = x, .y = y, .radius = 0, .max_radius = max_radius, .alpha = 255, .color = color};
            break;
        }
    }
}

static void create_particles(int16_t x, int16_t y, lv_color_t color, uint8_t count) {
    int created = 0;
    for (int i = 0; i < MAX_PARTICLES && created < count; i++) {
        if (particles[i].life == 0) {
            float angle = (rand() % 360) * M_PI / 180.0f;
            float speed = 1 + (rand() % 3);
            particles[i].vx = (int16_t)(cos(angle) * speed);
            particles[i].vy = (int16_t)(sin(angle) * speed);
            particles[i].life = 60 + (rand() % 60);
            particles[i].max_life = particles[i].life;
            particles[i].x = x;
            particles[i].y = y;
            particles[i].color = color;
            particles[i].size = 2 + (rand() % 3);
            created++;
        }
    }
}

static void update_particles(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].x += (current_wind_speed / 10);
            particles[i].vy += 1; // Gravity
            if (particles[i].x < 0 || particles[i].x >= CANVAS_WIDTH || particles[i].y < 0 || particles[i].y >= CANVAS_HEIGHT) {
                particles[i].life = 0;
            }
            particles[i].life--;
        }
    }
}

static void update_ripples(void) {
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (ripples[i].active) {
            ripples[i].radius += 3;
            ripples[i].alpha = (uint8_t)(255 * (1.0f - (float)ripples[i].radius / ripples[i].max_radius));
            if (ripples[i].radius >= ripples[i].max_radius) {
                ripples[i].active = false;
            }
        }
    }
}

static lv_obj_t * particle_objects[MAX_PARTICLES];
static lv_obj_t * ripple_objects[MAX_RIPPLES];

static void create_visual_objects(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        particle_objects[i] = lv_obj_create(main_canvas);
        lv_obj_set_size(particle_objects[i], 4, 4);
        lv_obj_set_style_radius(particle_objects[i], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(particle_objects[i], 0, LV_PART_MAIN);
        lv_obj_add_flag(particle_objects[i], LV_OBJ_FLAG_HIDDEN);
    }
    for (int i = 0; i < MAX_RIPPLES; i++) {
        ripple_objects[i] = lv_obj_create(main_canvas);
        lv_obj_set_style_radius(ripple_objects[i], LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(ripple_objects[i], LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_border_width(ripple_objects[i], 2, LV_PART_MAIN);
        lv_obj_add_flag(ripple_objects[i], LV_OBJ_FLAG_HIDDEN);
    }
}

static void update_visual_objects(void) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].life > 0) {
            lv_obj_clear_flag(particle_objects[i], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_pos(particle_objects[i], particles[i].x - 2, particles[i].y - 2);
            uint8_t alpha = (particles[i].life * 255) / particles[i].max_life;
            lv_obj_set_style_bg_color(particle_objects[i], particles[i].color, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(particle_objects[i], alpha, LV_PART_MAIN);
        } else {
            lv_obj_add_flag(particle_objects[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    for (int i = 0; i < MAX_RIPPLES; i++) {
        if (ripples[i].active) {
            lv_obj_clear_flag(ripple_objects[i], LV_OBJ_FLAG_HIDDEN);
            int16_t size = ripples[i].radius * 2;
            lv_obj_set_size(ripple_objects[i], size, size);
            lv_obj_set_pos(ripple_objects[i], ripples[i].x - ripples[i].radius, ripples[i].y - ripples[i].radius);
            lv_obj_set_style_border_color(ripple_objects[i], ripples[i].color, LV_PART_MAIN);
            lv_obj_set_style_border_opa(ripple_objects[i], ripples[i].alpha, LV_PART_MAIN);
        } else {
            lv_obj_add_flag(ripple_objects[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void animation_callback(lv_timer_t * timer) {
    update_particles();
    update_ripples();
    update_center_orb();
    update_visual_objects();
}

void WindChimeStartAnimation(void) {
    if (!animation_timer) {
        uint32_t period = 1000 / WINDCHIME_ANIMATION_FPS;
        animation_timer = lv_timer_create(animation_callback, period, NULL);
    }
}

void WindChimeStopAnimation(void) {
    if (animation_timer) {
        lv_timer_del(animation_timer);
        animation_timer = NULL;
    }
}