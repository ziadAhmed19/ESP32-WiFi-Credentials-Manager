#include <stdio.h>
#include "webInterface.h"

//use a struct to hold the SSID and PASSWORD
char STA_SSID[50] =             "WIFI_NETWORK_SSID";
char STA_PASSWORD[50] =         "WIFI_NETWORK_PASSWORD";

char *TAG = "webInterface";

bool wait_for_wifi_stopped(uint32_t timeout_ms)
{
    wifi_mode_t mode;
    uint32_t elapsed = 0;
    const uint32_t step = 100; // 100 ms step

    while (elapsed < timeout_ms)
    {
        if (esp_wifi_get_mode(&mode) == ESP_OK)
        {
            if (mode == WIFI_MODE_NULL)
            {
                return true; // fully stopped
            }
        }

        vTaskDelay(pdMS_TO_TICKS(step));
        elapsed += step;
    }

    return false; // timeout
}

bool bcheckWifi(void)
{
    wifi_ap_record_t ap_info;
    esp_err_t condition = esp_wifi_sta_get_ap_info(&ap_info);

    if (condition == ESP_ERR_WIFI_CONN || condition == ESP_ERR_WIFI_NOT_CONNECT || condition == ESP_ERR_WIFI_NOT_INIT)
    {
        ESP_LOGW(TAG, "WiFi Not Connected\n");
        webInterfaceTaskPriority = WEBINTERFACE_TASK_PRIORITY_HIGH;
        return false;
    } 
    
    else if (condition == ESP_OK)
    {
        ESP_LOGI(TAG, "WiFi Connected\n");
        webInterfaceTaskPriority = WEBINTERFACE_TASK_PRIORITY_LOW;
        return true;
    }
    else 
    {
        ESP_LOGE(TAG, "Some other error\n");
        //ESP_LOGE(TAG, "Error code: %s\n", esp_err_to_name(condition));
        ESP_ERROR_CHECK(condition);
        return false;
    }
}

void wifi_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_START: Station mode started, connecting to AP...");
                esp_wifi_connect(); // Trigger connection attempt
                break;
            case WIFI_EVENT_STA_CONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED: Connected to AP SSID:%s", STA_SSID);
                // Note: Channel is now fixed by the AP we connected to.
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED: Disconnected from AP, attempting reconnect...");
                esp_wifi_connect(); // Attempt to reconnect
                break;
            case WIFI_EVENT_AP_START:
                ESP_LOGI(TAG, "WIFI_EVENT_AP_START: SoftAP started, SSID:%s", AP_SSID);
                // Log the actual channel the AP is using (which is dictated by STA)
                wifi_config_t conf;
                esp_wifi_get_config(WIFI_IF_AP, &conf);
                ESP_LOGI(TAG, "AP Channel: %d", conf.ap.channel);
                break;
            case WIFI_EVENT_AP_STACONNECTED: {
                //wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
                //ESP_LOGI(TAG, "WIFI_EVENT_AP_STACONNECTED: Station "%s" joined, AID=%d",MAC2STR(event->mac), event->aid);
                break;
            }
            case WIFI_EVENT_AP_STADISCONNECTED: {
                //wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
                //ESP_LOGI(TAG, "WIFI_EVENT_AP_STADISCONNECTED: Station "%s" left, AID=%d",MAC2STR(event->mac), event->aid);
                break;
            }

            default:
                ESP_LOGI(TAG, "Unhandled WIFI_EVENT: %ld", event_id);
                break;
        }
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP: {
                //ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
                //ESP_LOGI(TAG, "IP_EVENT_STA_GOT_IP: Got IP address from AP:" IPSTR, IP2STR(&event->ip_info.ip));
                // You can now use the STA interface for network communication
                break;
            }
            case IP_EVENT_AP_STAIPASSIGNED: { // Optional: Log IP assigned by ESP32's DHCP server
                 //ip_event_ap_staipassigned_t* event = (ip_event_ap_staipassigned_t*) event_data;
                 //ESP_LOGI(TAG, "IP_EVENT_AP_STAIPASSIGNED: Assigned IP " IPSTR " to station", IP2STR(&event->ip));
                 break;
             }
            default:
                ESP_LOGI(TAG, "Unhandled IP_EVENT: %ld", event_id);
                break;
        }
    }
}

void webInterface_APSTA_init(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    //Initialize TCP/IP Stack
    ESP_ERROR_CHECK(esp_netif_init());
    
    //Create Event Loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    //Create Network Interfaces
    esp_netif_t *sta_netif  = esp_netif_create_default_wifi_sta();
    esp_netif_t *ap_netif   = esp_netif_create_default_wifi_ap();
    assert(sta_netif);
    assert(ap_netif);

    //Initialize WiFi Stack
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //Register Event Handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    
    // Set WiFi Mode to APSTA
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    // Configure AP and STA Parameters
    wifi_config_t wifi_config = 
    {
        .ap =
        {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = 1,
            .password = AP_PASSWORD,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
        .sta = 
        {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        }
    };
    // Setting up the new SSID and password
    strncpy((char*)wifi_config.sta.ssid, STA_SSID, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, STA_PASSWORD, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());

    
    ESP_LOGI(TAG, "wifi_init_apsta finished.");
}

void webInterface_APSTA_reinit(char* ssid, char* password)
{
    // Configure AP and STA Parameters
    wifi_config_t wifi_config = 
    {
        .ap =
        {
            .ssid = AP_SSID,
            .ssid_len = strlen(AP_SSID),
            .channel = 1,
            .password = AP_PASSWORD,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
        .sta = 
        {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        }
    };
    // Setting up the new SSID and password
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Start WiFi
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "wifi_re-init_apsta finished.");
}

esp_err_t WiFiConfiguration_POST_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int received = 0;
    int ret;

    // Allocate buffer for POST data (+1 for null terminator)
    char *post_buffer = (char *)pvPortMalloc(total_len + 1);
    if (!post_buffer) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Read the POST data in chunks until done
    while (received < total_len) {
        ret = httpd_req_recv(req, post_buffer + received, total_len - received);
        if (ret <= 0) {  // error or timeout
            vPortFree(post_buffer);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        received += ret;
    }

    // Null-terminate the buffer
    post_buffer[received] = '\0';

    ESP_LOGI(TAG, "Received Data (%d bytes): %s", received, post_buffer);

    // Process the received POST data
    WiFiConfiguration_POST_Process_Request(post_buffer);

    // Send response to the client
    const char *response = "WiFi configuration updated.";
    httpd_resp_send(req, response, HTTPD_RESP_USE_STRLEN);

    vPortFree(post_buffer);
    return ESP_OK;
}

static char from_hex(char c) {
// Helper function to convert two hex chars to a byte
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

static void url_decode(char *dst, const char *src) {
// URL decode function: turns %20 to space
    char a, b;
    while (*src) {
        if ((*src == '%') && ((a = src[1]) && (b = src[2])) && (isxdigit(a) && isxdigit(b))) {
            *dst++ = from_hex(a) * 16 + from_hex(b);
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

esp_err_t WiFiConfiguration_POST_Process_Request(char* sPostBuffer)
{
    char raw_ssid[100] = {0};
    char raw_password[100] = {0};
    char ssid[50] = {0};
    char password[50] = {0};

    // Find where "ssid=" and "password=" start
    char *ssid_ptr = strstr(sPostBuffer, "ssid=");
    char *pass_ptr = strstr(sPostBuffer, "password=");

    // Fetching SSID
    if (ssid_ptr) {
        ssid_ptr += 5; // skip "ssid="
        char *end = strchr(ssid_ptr, '&');
        if (end) {
            // This line puts a cap to the size of the SSID to avoid overflow 
            // It cannot be bigger that the raw_ssid size -1 for null termination
            size_t len = MIN(end - ssid_ptr, sizeof(raw_ssid) - 1);
            strncpy(raw_ssid, ssid_ptr, len);
            raw_ssid[len] = '\0';
        } else {
            strncpy(raw_ssid, ssid_ptr, sizeof(raw_ssid) - 1);
        }
    }

    // Fetching Password
    if (pass_ptr) {
        pass_ptr += 9; // skip "password="
        strncpy(raw_password, pass_ptr, sizeof(raw_password) - 1);
    }

    // Decode URL encoding (My%20WiFi → My WiFi)
    url_decode(ssid, raw_ssid);
    url_decode(password, raw_password);

    ESP_LOGI(TAG, "Parsed SSID: '%s'", ssid);
    ESP_LOGI(TAG, "Parsed Password: '%s'", password);
    
    // Store the previous SSID and password in a temp var
    char *previous_ssid = (char*)pvPortMalloc(sizeof(char) * 50);
    if(previous_ssid != NULL) {
        memset(previous_ssid, '\0', sizeof(char)*50);
    }
    previous_ssid = strcpy(previous_ssid, STA_SSID);

    char *previous_password = (char*)pvPortMalloc(sizeof(char) * 50);
    if(previous_password != NULL) {
        memset(previous_password, '\0', sizeof(char)*50);
    }
    previous_password = strcpy(previous_password, STA_PASSWORD);

    // Trigger disconnect with new credentials
    esp_wifi_disconnect();
    esp_wifi_stop();
    
    // Wait for 1 second to ensure WiFi is stopped
    vTaskDelay(1000 / portTICK_PERIOD_MS); 
    
    // Reinitialize WiFi with new SSID and Password
    webInterface_APSTA_reinit(ssid, password);
    
    // Attempt to connect with new credentials
    __uint8_t connection_attempts = 0;

    while (!bcheckWifi() && connection_attempts < WIFI_MAX_RETRIES)
    {
        ESP_LOGI(TAG, "Attempting to connect... (%d/%d)", connection_attempts + 1, WIFI_MAX_RETRIES);
        esp_wifi_connect();
        vTaskDelay(5000 / portTICK_PERIOD_MS); // Wait for 5 seconds before next attempt
        connection_attempts++;
    }
    // If connection failed, revert to previous credentials
    if (!bcheckWifi())
    {
        ESP_LOGW(TAG, "Failed to connect with new credentials. Reverting to previous SSID and Password.");
        
        // Revert to previous SSID and Password
        esp_wifi_stop();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        webInterface_APSTA_reinit(previous_ssid, previous_password);
    }
    else
    {
        strcpy(STA_SSID, ssid);
        strcpy(STA_PASSWORD, password);
    }

    return ESP_OK;
}

esp_err_t root_get_handler(httpd_req_t *req) {
    const char *html = "<html><body>"
                       "<h2>Change WiFi SSID and Password</h2>"
                       "<form action=\"/WiFi-configuration\" method=\"POST\">"
                       "<input type=\"text\" name=\"ssid\" placeholder=\"Enter SSID\" required>"
                       "<input type=\"password\" name=\"password\" placeholder=\"Enter Password\" required>"
                       "<button type=\"submit\">Submit</button>"
                       "</form>"
                       "</body></html>";

    httpd_resp_send(req, html, strlen(html));
    return ESP_OK;
}

httpd_handle_t webInteface_start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    
    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        // Registering URI handlers
        httpd_uri_t post_uri = {
            .uri       = "/WiFi-configuration",             // Endpoint for incoming requests
            .method    = HTTP_POST,                         // Accept POST requests
            .handler   = WiFiConfiguration_POST_handler, // Function that processes requests
            .user_ctx  = NULL
        };        
        httpd_register_uri_handler(server, &post_uri);
        
        httpd_uri_t root_uri = {
            .uri       = "/", // Root URI serves the HTML form
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &root_uri);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}



void webInterface_Task_Test(void *pvParameters)
{

    while(1)
    {

    }
}