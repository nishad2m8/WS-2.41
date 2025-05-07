#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "ui.h"
#include "button_logic.h"
#include "wifi_mqtt.h"

WiFiClient espClient;
PubSubClient client(espClient);

// Forward declare helper function (internal to this file)
lv_event_t *createSimulatedEvent(lv_obj_t *target);
void freeSimulatedEvent(lv_event_t *e);

void setup_wifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
}

void setup_mqtt() {
    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(mqtt_callback);
}

void mqtt_loop() {
    if (!client.connected()) {
        while (!client.connected()) {
            if (client.connect(MQTT_CLIENT_ID)) {
                // Subscribe to AC topics
                client.subscribe(TOPIC_AC_SET);
                client.subscribe("home/ac/get");
                client.subscribe("home/all/get");
                
                // Send initial state to sync with Node-RED dashboard
                publishAllStates();
            } else {
                delay(2000);
            }
        }
    }
    client.loop();
}

// Helper function to publish all current states to MQTT to sync dashboard
void publishAllStates() {
    // Get current AC state using the state retrieval function
    int acValue = get_ac_state();
    
    // Publish state with retain flag set to true
    publish_ac_state(acValue);
}

// MQTT callback to update UI when messages are received
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    // For zero-length messages (like state requests), we'll still process them
    String message = "";
    if (length > 0) {
        payload[length] = '\0';
        message = String((char*)payload);
    }
    
    String topicStr = String(topic);
    
    // Handle AC state update topic
    if (topicStr == TOPIC_AC_SET) {
        int value = message.toInt();
        // Create a proper event for AC slider
        lv_obj_t *slider = ui_Slider_AC;
        lv_slider_set_value(slider, value, LV_ANIM_OFF);
        
        // Simulate event to update UI properly
        lv_event_t *e = createSimulatedEvent(slider);
        ac_slider_event_cb(e);
        freeSimulatedEvent(e);
    } 
    // Handle state request topics (the "get" topics)
    else if (topicStr == "home/ac/get") {
        publish_ac_state(get_ac_state());
    }
    else if (topicStr == "home/all/get") {
        publishAllStates();
    }
}

// Helper function to create a simulated event with a target
lv_event_t *createSimulatedEvent(lv_obj_t *target) {
    lv_event_t *e = (lv_event_t*)malloc(sizeof(lv_event_t));
    if (e) {
        e->target = target;
        e->current_target = target;
        e->code = LV_EVENT_VALUE_CHANGED;
        e->user_data = NULL;
        e->param = NULL;
    }
    return e;
}

// Free the simulated event
void freeSimulatedEvent(lv_event_t *e) {
    if (e) {
        free(e);
    }
}

// Used from logic code to publish state
void publish_ac_state(int value) {
    client.publish(TOPIC_AC_STATE, String(value).c_str(), true); // Set retain flag to true
}