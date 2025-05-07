#include <Arduino.h>
#include <stdio.h>
#include "esp_log.h"

#include "display_controller.h"
#include "ui.h"
#include "button_logic.h"
#include "wifi_mqtt.h"

static const char *TAG = "main";

// Display controller instance
static DisplayController *display = nullptr;

void setup() {
    ESP_LOGI(TAG, "Starting application");

    //Initialize Wi-Fi and MQTT first
    setup_wifi();
    setup_mqtt();

    //Initialize the display
    display = new DisplayController();
    display->init();

    if (DisplayController::lvgl_lock()) {
        ui_init();  // UI init from SquareLine Studio

        setup_button_callbacks();  // Register button and slider callbacks

        DisplayController::lvgl_unlock();
    }

    //Publish initial AC state to Node-RED
    publishAllStates();
}

void loop() {
    delay(5);  // Prevent watchdog timeout

    //Run MQTT processing loop (handles reconnects, callbacks)
    mqtt_loop();
}
