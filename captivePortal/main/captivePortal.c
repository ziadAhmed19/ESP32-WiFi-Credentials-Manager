#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "webInterface.h"

char *mainTAG = "CaptivePortal";


void app_main(void)
{   
    webInterface_APSTA_init();
    webInteface_start_webserver();
}