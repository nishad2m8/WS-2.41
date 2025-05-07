#include "button_logic.h"
#include "ui.h"
#include "wifi_mqtt.h"
#include <stdio.h>

#define AC_TEMP_MIN 19
#define AC_TEMP_MAX 26
#define AC_TEMP_DEFAULT 20

// Visibility state for each device
static bool lamp_visible = false;
static bool tv_visible = true;
static bool ac_visible = false;
static bool vacuum_visible = false;

// Toggle Lamp
static void toggle_lamp() {
    lamp_visible = !lamp_visible;
    if (lamp_visible) {
        lv_obj_clear_flag(ui_Image_Lux, LV_OBJ_FLAG_HIDDEN);
        int32_t lamp_value = lv_slider_get_value(ui_Slider_Lamp);
        if (lamp_value == 0) {
            lamp_value = 50;
            lv_slider_set_value(ui_Slider_Lamp, lamp_value, LV_ANIM_ON);
            static char buf[8];
            snprintf(buf, sizeof(buf), "%ld%%", lamp_value);
            lv_label_set_text(ui_Label_Lux, buf);
        }
        uint8_t opacity = (uint8_t)((lamp_value * 255) / 100);
        lv_obj_set_style_img_opa(ui_Image_Lux, opacity, LV_PART_MAIN | LV_STATE_DEFAULT);
    } else {
        lv_slider_set_value(ui_Slider_Lamp, 0, LV_ANIM_ON);
        lv_label_set_text(ui_Label_Lux, "0%");
        lv_obj_set_style_img_opa(ui_Image_Lux, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_add_flag(ui_Image_Lux, LV_OBJ_FLAG_HIDDEN);
    }
}

// Toggle TV
static void toggle_tv() {
    tv_visible = !tv_visible;
    if (tv_visible) {
        lv_obj_clear_flag(ui_Image_TV_Off, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui_Image_TV_Off, LV_OBJ_FLAG_HIDDEN);
    }
}

// Toggle AC
static void toggle_ac() {
    ac_visible = !ac_visible;
    if (ac_visible) {
        lv_obj_clear_flag(ui_Image_AC_On, LV_OBJ_FLAG_HIDDEN);
        int32_t ac_value = lv_slider_get_value(ui_Slider_AC);
        if (ac_value == 0 || ac_value < AC_TEMP_MIN) {
            ac_value = AC_TEMP_DEFAULT;
            lv_slider_set_value(ui_Slider_AC, ac_value, LV_ANIM_ON);
        }
        if (ac_value > AC_TEMP_MAX) {
            ac_value = AC_TEMP_MAX;
            lv_slider_set_value(ui_Slider_AC, ac_value, LV_ANIM_ON);
        }
        static char buf[10];
        snprintf(buf, sizeof(buf), "%ld°", ac_value);
        lv_label_set_text(ui_Label_Temp, buf);

        publish_ac_state(ac_value);
    } else {
        lv_slider_set_value(ui_Slider_AC, 0, LV_ANIM_ON);
        lv_label_set_text(ui_Label_Temp, "0°");
        lv_obj_add_flag(ui_Image_AC_On, LV_OBJ_FLAG_HIDDEN);

        publish_ac_state(0);
    }
}

// Toggle Vacuum
static void toggle_vacuum() {
    vacuum_visible = !vacuum_visible;
    if (vacuum_visible) {
        lv_obj_clear_flag(ui_Image_Vacuumoff, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(ui_Image_Vacuumoff, LV_OBJ_FLAG_HIDDEN);
    }
}

// Button Callbacks
void lamp_event_cb(lv_event_t * e) { toggle_lamp(); }
void tv_event_cb(lv_event_t * e) { toggle_tv(); }
void ac_event_cb(lv_event_t * e) { toggle_ac(); }
void vacuum_event_cb(lv_event_t * e) { toggle_vacuum(); }

// Lamp Slider
void lamp_slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);
    static char buf[8];
    snprintf(buf, sizeof(buf), "%ld%%", value);
    lv_label_set_text(ui_Label_Lux, buf);

    if (value == 0 && lamp_visible) {
        toggle_lamp();
        return;
    }

    if (lamp_visible) {
        uint8_t opacity = (uint8_t)((value * 255) / 100);
        lv_obj_set_style_img_opa(ui_Image_Lux, opacity, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}

// AC Slider
void ac_slider_event_cb(lv_event_t * e) {
    lv_obj_t * slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    if (value == 0 && ac_visible) {
        toggle_ac();
        return;
    }

    if (value > 0 && value < AC_TEMP_MIN) {
        value = AC_TEMP_MIN;
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
    } else if (value > AC_TEMP_MAX) {
        value = AC_TEMP_MAX;
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
    }

    static char buf[10];
    snprintf(buf, sizeof(buf), "%ld°", value);
    lv_label_set_text(ui_Label_Temp, buf);

    publish_ac_state(value);
}

// Return current AC value
int get_ac_state() {
    return lv_slider_get_value(ui_Slider_AC);
}

// Register all callbacks
void setup_button_callbacks() {
    lv_obj_add_event_cb(ui_Button_Lamp, lamp_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Button_TV, tv_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Button_AC, ac_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(ui_Button_VC, vacuum_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_add_event_cb(ui_Slider_Lamp, lamp_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(ui_Slider_AC, ac_slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    int32_t lamp_value = lv_slider_get_value(ui_Slider_Lamp);
    int32_t ac_value = lv_slider_get_value(ui_Slider_AC);

    static char lamp_buf[8];
    static char ac_buf[10];

    snprintf(lamp_buf, sizeof(lamp_buf), "%ld%%", lamp_value);
    snprintf(ac_buf, sizeof(ac_buf), "%ld°", ac_value);

    lv_label_set_text(ui_Label_Lux, lamp_buf);
    lv_label_set_text(ui_Label_Temp, ac_buf);

    uint8_t opacity = (uint8_t)((lamp_value * 255) / 100);
    lv_obj_set_style_img_opa(ui_Image_Lux, opacity, LV_PART_MAIN | LV_STATE_DEFAULT);

    if (!lamp_visible) lv_obj_add_flag(ui_Image_Lux, LV_OBJ_FLAG_HIDDEN);
    if (!tv_visible) lv_obj_add_flag(ui_Image_TV_Off, LV_OBJ_FLAG_HIDDEN);
    if (!ac_visible) lv_obj_add_flag(ui_Image_AC_On, LV_OBJ_FLAG_HIDDEN);
    if (!vacuum_visible) lv_obj_add_flag(ui_Image_Vacuumoff, LV_OBJ_FLAG_HIDDEN);
}
