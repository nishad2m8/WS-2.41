#ifndef WIFI_MQTT_H
#define WIFI_MQTT_H

#include <stdint.h> // For standard integer types

// Define byte type if not already defined
#ifndef byte
typedef uint8_t byte;
#endif

void setup_wifi();
void setup_mqtt();
void mqtt_loop();

int get_ac_state();

// State publishing function
void publish_ac_state(int value);

// State synchronization function
void publishAllStates();

// MQTT callback function
void mqtt_callback(char* topic, byte* payload, unsigned int length);

#endif // WIFI_MQTT_H