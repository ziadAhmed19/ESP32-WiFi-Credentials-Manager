# Captive Portal

## Web Interface Page

<html>
  <body>
    <h2>Change WiFi SSID and Password</h2>
    <form action="/WiFi-configuration" method="POST">
      <input type="text" name="ssid" placeholder="Enter SSID" required>
      <input type="password" name="password" placeholder="Enter Password" required>
      <button type="submit">Submit</button>
    </form>
  </body>
</html>

## Prerequisites

- ESP-IDF v5.1.X

## Project Overview

Captive Portal is a small ESP32 project that exposes a web interface to set or change WiFi credentials (SSID and password).

The device runs in AP mode when not connected, serving a captive portal at `http://192.168.4.1/`, and continues to serve an HTTP endpoint while connected to allow changing credentials remotely.

### Accessing the Captive Portal

If your ESP32 is **not connected to any network**, type the following URL in your web browser:
http://192.168.4.1/


If your ESP32 is **connected to a network**, find its current IP address and use:
http://esp-current-ip/


You can also use `curl` to change the WiFi credentials using:

```bash
curl -X POST -d "ssid=WiFi_SSID&password=WiFi_PASSWORD" http://ESP-CURRENT-IP/WiFi-configuration
```

If the ESP32 is not connected to any network, use:

```bash
curl -X POST -d "ssid=WiFi_SSID&password=WiFi_PASSWORD" http://192.168.4.1/WiFi-configuration
```

## How Does It Work

Under the hood, we start by initializing the ESP32 in **APSTA mode** (Access Point + Station Mode) by calling:

```c
webInterface_APSTA_init();
```

Next, we start our web server with:

```c
webInteface_start_webserver();
```

### URI Handlers

We register our URI handlers, which include:

- **Root URI**: This serves the HTML webpage with its corresponding handler function.
- **WiFi-configuration URI**: This functions as an endpoint for incoming POST requests.

The core functionality of this project is handled within the appropriate handler function, where we fetch the incoming POST request from the user and store it in the **post_buffer**. This buffer is then passed to:

```c
WiFiConfiguration_POST_Process_Request(post_buffer);
```

### Processing the POST Request

Within `WiFiConfiguration_POST_Process_Request`, we process the post buffer by utilizing the `strstr()` and `strchr()` functions to extract the **SSID** and **password**.

Next, we temporarily store the current SSID and password of the ESP32 in variables so we can revert back to them if the provided credentials are incorrect.

Here's relevant code for fetching the SSID and password:

```c
char *ssid_ptr = strstr(sPostBuffer, "ssid=");
char *pass_ptr = strstr(sPostBuffer, "password=");
```

### Steps for Credential Update

1. Store the previous SSID and password in temporary variables.
2. Trigger the ESP32 to disconnect from the WiFi network.
3. Update the credentials using `wifi_config_t`.
4. Attempt to reconnect with `esp_wifi_connect()`.
5. If the connection fails, revert to the previous SSID and password.

### Debugging Tips
- Make sure to monitor the logs using `ESP_LOGI` and `ESP_LOGW` to understand the flow of WiFi connection attempts.
- Verify that all pointers and buffers are correctly allocated and freed to prevent memory leaks.

This revised explanation aims for clarity and more logical flow, helping others understand your project's workings better.