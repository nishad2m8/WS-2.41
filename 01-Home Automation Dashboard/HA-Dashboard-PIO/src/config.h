#ifndef CONFIG_H
#define CONFIG_H

// Wi-Fi
#define WIFI_SSID     "SSID"
#define WIFI_PASSWORD "PASSWORD"

// MQTT
#define MQTT_BROKER   "192.168.100.41"
#define MQTT_PORT     1883
#define MQTT_CLIENT_ID "esp32_ui_client"

// MQTT Topics
#define TOPIC_AC_SET      "home/ac/set"
#define TOPIC_AC_STATE    "home/ac/state"

#endif // CONFIG_H