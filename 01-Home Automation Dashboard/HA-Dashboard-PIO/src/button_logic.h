#ifndef BUTTON_LOGIC_H
#define BUTTON_LOGIC_H

#include "lvgl.h"

// Define constants for AC temperature limits
int get_ac_state();

// Event callback functions for each button
void ac_event_cb(lv_event_t * e);

// Slider event callbacks
void ac_slider_event_cb(lv_event_t * e);

// Function to set up all button callbacks
void setup_button_callbacks();

#endif // BUTTON_LOGIC_H