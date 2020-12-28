/*
Autor Hênio Reis
Proj. Esp32cam e protocolo mqtt nativo idf
Obs: aplicação de envio de imagem para node red para fins IA.
Data: 15/08/2020
*/
#include "mqtt_client.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event_loop.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "init_start_camera.h"
#include "esp_camera.h"

#define PWDN_GPIO_NUM 26

const char *ssid = ".......";
const char *password = ".......";
const char *uri = ".............."; // mqtt://mqtt.eclipse.org
//const char *user = "";
//const char *pass = "";
//int port = 33009;

uint8_t *pic_buf;
uint8_t * _jpg_buf;
size_t length;
esp_err_t res = ESP_OK;
camera_fb_t *fb = NULL;

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;

static esp_err_t mqtt_event_handler (esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    int i = 0;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            Serial.printf( "MQTT_EVENT_CONNECTED\n");
            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            Serial.printf("sent subscribe successful, msg_id=%d \n", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            //Serial.printf( "MQTT_EVENT_DISCONNECTED");
            esp_mqtt_client_reconnect(client);
            break;

        case MQTT_EVENT_SUBSCRIBED:
            Serial.printf("MQTT_EVENT_SUBSCRIBED, msg_id=%d \n", event->msg_id);
            printf("Taking picture now");
            
           
            fb = esp_camera_fb_get();  
            if(!fb)
            {
            printf("Camera capture failed");
            return false;
            }
            
            printf("Camera capture success\n\r");
            pic_buf = fb->buf;
            length = fb->len;
            
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", (const char*)pic_buf, length, 0, 0);
            Serial.printf( "sent publish successful, msg_id=%d \n\r", msg_id);
            
            esp_camera_fb_return(fb);
            
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            Serial.printf( "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d \n", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            Serial.printf( "MQTT_EVENT_PUBLISHED, msg_id=%d \n", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            //Serial.printf("MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            Serial.printf("MQTT_EVENT_ERROR");
            break;
        default:
            Serial.printf("Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

// Wifi event handler
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    
  case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    
  case SYSTEM_EVENT_STA_DISCONNECTED:
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    
  default:
        break;
    }
   
  return ESP_OK;
}


static void mqtt_app_start(void)
{   
   
    esp_mqtt_client_config_t mqtt_cfg;
    memset(&mqtt_cfg, 0, sizeof(mqtt_cfg));
    mqtt_cfg.uri = uri;
    mqtt_cfg.refresh_connection_after_ms = 10000;
    mqtt_cfg.event_handle = mqtt_event_handler;
 
    //mqtt_cfg.port = port;
    //if (strlen(user)) {mqtt_cfg.username  = user;}
    //if (strlen(pass)) {mqtt_cfg.password  = pass;}
    
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_start(client);
    
}

void wifi_initialise(void) {
  
  // initialize NVS, required for wifi
  nvs_flash_init();
    
  // connect to the wifi network
  wifi_event_group = xEventGroupCreate();
  tcpip_adapter_init();
  esp_event_loop_init(event_handler, NULL);
  wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wifi_init_config);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_mode(WIFI_MODE_STA);
  wifi_config_t wifi_config = {};
  strcpy((char*)wifi_config.sta.ssid, ssid);
  strcpy((char*)wifi_config.sta.password, password);
  wifi_config.sta.bssid_set = false;
  
  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
  esp_wifi_start();
}

void wifi_wait_connected()
{
  xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

void setup() {
  pinMode(PWDN_GPIO_NUM, PULLUP);
  digitalWrite(PWDN_GPIO_NUM, HIGH);
  Serial.begin(115200);
  
  START_ESP32CAM();
  wifi_initialise();
  wifi_wait_connected();
  mqtt_app_start();

}

void loop() {

}
