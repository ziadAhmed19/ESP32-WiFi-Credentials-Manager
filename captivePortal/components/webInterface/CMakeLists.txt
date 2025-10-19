idf_component_register(SRCS "webInterface.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_wifi
                    REQUIRES esp_event
                    REQUIRES esp_netif
                    REQUIRES nvs_flash
                    REQUIRES esp_http_server
                    )
