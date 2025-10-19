#ifndef WEBINTERFACE_H
#define WEBINTERFACE_H
// INCLUDES
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs_flash.h"

// DEFINES
#define WEBINTERFACE_TASK_PRIORITY_HIGH 2
#define WEBINTERFACE_TASK_PRIORITY_LOW  configMAX_PRIORITIES-1

#define WIFI_MAX_RETRIES 5

#define AP_SSID                     "Proton"
#define AP_PASSWORD                 "password"

// MACROS
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// FUNCTION PROTOTYPES
bool bcheckWifi(void);
void webInterface_APSTA_init(void);
void webInterface_Task_Test(void *pvParameters);
void wifi_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
httpd_handle_t webInteface_start_webserver(void);
esp_err_t root_get_handler(httpd_req_t *req);
esp_err_t WiFiConfiguration_POST_handler(httpd_req_t *req);
esp_err_t WiFiConfiguration_POST_Process_Request(char* sPostBuffer);
void webInterface_APSTA_reinit(char* ssid, char* password);

#endif // WEBINTERFACE_H