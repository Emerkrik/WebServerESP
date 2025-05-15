#include <stdio.h>
#include <stdlib.h>
#include <string.h> //Requires by memset
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <esp_http_server.h>
#include "spi_flash_mmap.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "driver/gpio.h"
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/api.h>
#include <lwip/netdb.h>
#include "wifi_manager.h"
#include "mdns.h"

#define LED_PIN2 2
#define LED_PIN3 3
#define LED_PIN4 4
#define LED_PIN15 15

#define EXAMPLE_ESP_WIFI_SSID CONFIG_ESP_WIFI_SSID 

#define EXAMPLE_ESP_WIFI_PASS CONFIG_ESP_WIFI_PASSWORD 

#define EXAMPLE_ESP_MAXIMUM_RETRY CONFIG_ESP_MAXIMUM_RETRY 

//WEB SERVER


//html pagina
char on_resp[] = "<!DOCTYPE html><html><head><style type=\"text/css\">html {  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #b30000; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>ESP32 WEB SERVER</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\" ></head><body>  <h2>ESP32 WEB SERVER</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i>     <strong>GPIO2</strong></p>        <p>GPIO state: <strong> ON</strong></p>        <p>          <a href=\"/led2on\"><button class=\"button\">ON</button></a>          <a href=\"/led2off\"><button class=\"button button2\">OFF</button></a>        </p>      </div>    </div>  </div></body></html>"; 
char off_resp[] = "<!DOCTYPE html><html><head><style type=\"text/css\">html {  font-family: Arial;  display: inline-block;  margin: 0px auto;  text-align: center;}h1{  color: #070812;  padding: 2vh;}.button {  display: inline-block;  background-color: #b30000; //red color  border: none;  border-radius: 4px;  color: white;  padding: 16px 40px;  text-decoration: none;  font-size: 30px;  margin: 2px;  cursor: pointer;}.button2 {  background-color: #364cf4; //blue color}.content {   padding: 50px;}.card-grid {  max-width: 800px;  margin: 0 auto;  display: grid;  grid-gap: 2rem;  grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));}.card {  background-color: white;  box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);}.card-title {  font-size: 1.2rem;  font-weight: bold;  color: #034078}</style>  <title>ESP32 WEB SERVER</title>  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">  <link rel=\"icon\" href=\"data:,\">  <link rel=\"stylesheet\" href=\"https://use.fontawesome.com/releases/v5.7.2/css/all.css\"    integrity=\"sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr\" crossorigin=\"anonymous\">  <link rel=\"stylesheet\" type=\"text/css\"></head><body>  <h2>ESP32 WEB SERVER</h2>  <div class=\"content\">    <div class=\"card-grid\">      <div class=\"card\">        <p><i class=\"fas fa-lightbulb fa-2x\" style=\"color:#c81919;\"></i>     <strong>GPIO2</strong></p>        <p>GPIO state: <strong> OFF</strong></p>        <p>          <a href=\"/led2on\"><button class=\"button\">ON</button></a>          <a href=\"/led2off\"><button class=\"button button2\">OFF</button></a>        </p>      </div>    </div>  </div></body></html>"; 

static const char *TAG = "espressif"; // TAG for debug
int led_state = 0;

// Add this near the top of your file or in a config header
#define WIFI_MANAGER_MAX_RETRIES 5

static int wifi_manager_retry_count = 0;

// Add this near the top, after your includes:
static httpd_handle_t my_server_handle = NULL;


// Forward declarations for all HTTP handler functions
esp_err_t get_req_handler(httpd_req_t *req);
esp_err_t led_on_handler(httpd_req_t *req);
esp_err_t led_off_handler(httpd_req_t *req);
esp_err_t led3_on_handler(httpd_req_t *req);
esp_err_t led3_off_handler(httpd_req_t *req);
esp_err_t led4_on_handler(httpd_req_t *req);
esp_err_t led4_off_handler(httpd_req_t *req);
esp_err_t led15_on_handler(httpd_req_t *req);
esp_err_t led15_off_handler(httpd_req_t *req);


httpd_uri_t uri_get = { 
    .uri = "/", 
    .method = HTTP_GET, 
    .handler = get_req_handler, 
    .user_ctx = NULL}; 

httpd_uri_t uri_on = { 
    .uri = "/led2on", 
    .method = HTTP_GET, 
    .handler = led_on_handler, 
    .user_ctx = NULL}; 

httpd_uri_t uri_off = { 
    .uri = "/led2off", 
    .method = HTTP_GET, 
    .handler = led_off_handler, 
    .user_ctx = NULL}; 

httpd_uri_t uri_led3_on = { 
    .uri = "/led3on", 
    .method = HTTP_GET, 
    .handler = led3_on_handler, 
    .user_ctx = NULL};

httpd_uri_t uri_led3_off = {    
    .uri = "/led3off", 
    .method = HTTP_GET, 
    .handler = led3_off_handler, 
    .user_ctx = NULL};

httpd_uri_t uri_led4_on = {     
    .uri = "/led4on", 
    .method = HTTP_GET, 
    .handler = led4_on_handler, 
    .user_ctx = NULL};

httpd_uri_t uri_led4_off = {

    .uri = "/led4off", 
    .method = HTTP_GET, 
    .handler = led4_off_handler, 
    .user_ctx = NULL};

httpd_uri_t uri_led15_on = {     

    .uri = "/led15on", 
    .method = HTTP_GET, 
    .handler = led15_on_handler, 
    .user_ctx = NULL};

httpd_uri_t uri_led15_off = {

    .uri = "/led15off", 
    .method = HTTP_GET, 
    .handler = led15_off_handler, 
    .user_ctx = NULL}; 


// Implement all HTTP handler functions
esp_err_t get_req_handler(httpd_req_t *req) {
    // Serve the main page depending on led_state
    if (led_state) {
        httpd_resp_send(req, on_resp, strlen(on_resp));
    } else {
        httpd_resp_send(req, off_resp, strlen(off_resp));
    }
    return ESP_OK;
}

esp_err_t led_on_handler(httpd_req_t *req) {
    gpio_set_level(LED_PIN2, 1);
    led_state = 1;
    httpd_resp_send(req, on_resp, strlen(on_resp));
    return ESP_OK;
}

esp_err_t led_off_handler(httpd_req_t *req) {
    gpio_set_level(LED_PIN2, 0);
    led_state = 0;
    httpd_resp_send(req, off_resp, strlen(off_resp));
    return ESP_OK;
}

esp_err_t led3_on_handler(httpd_req_t *req) {
    gpio_set_level(LED_PIN3, 1);
    httpd_resp_send(req, "LED3 ON", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t led3_off_handler(httpd_req_t *req) {
    gpio_set_level(LED_PIN3, 0);
    httpd_resp_send(req, "LED3 OFF", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t led4_on_handler(httpd_req_t *req) {
    gpio_set_level(LED_PIN4, 1);
    httpd_resp_send(req, "LED4 ON", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t led4_off_handler(httpd_req_t *req) {
    gpio_set_level(LED_PIN4, 0);
    httpd_resp_send(req, "LED4 OFF", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t led15_on_handler(httpd_req_t *req) {
    gpio_set_level(LED_PIN15, 1);
    httpd_resp_send(req, "LED15 ON", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t led15_off_handler(httpd_req_t *req) {
    gpio_set_level(LED_PIN15, 0);
    httpd_resp_send(req, "LED15 OFF", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

//SETUP_SERVER
// Function to set up the HTTP server

httpd_handle_t setup_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    httpd_handle_t server = NULL;
config.max_uri_handlers = 20;
config.uri_match_fn = httpd_uri_match_wildcard; // Optional, for flexible URI matching

    if (httpd_start(&server, &config) == ESP_OK)
    {
        httpd_register_uri_handler(server, &uri_get); 
        httpd_register_uri_handler(server, &uri_on); 
        httpd_register_uri_handler(server, &uri_off); 
        httpd_register_uri_handler(server, &uri_led3_on); 
        httpd_register_uri_handler(server, &uri_led3_off); 
        httpd_register_uri_handler(server, &uri_led4_on); 
        httpd_register_uri_handler(server, &uri_led4_off); 
        httpd_register_uri_handler(server, &uri_led15_on); 
        httpd_register_uri_handler(server, &uri_led15_off); 
    }

    return server;
}

// Add mDNS initialization callback
void cb_connection_ok(void *pvParameter) {
    // Initialize mDNS when WiFi is connected
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set(DEFAULT_HOSTNAME));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32 WebServer"));
    ESP_LOGI("mDNS", "mDNS started with hostname: %s", DEFAULT_HOSTNAME);

    // Stop wifi_manager HTTP server (provisioning page)
    extern void http_app_stop(void);
    http_app_stop();

    // Start your own HTTP server if not already running
    if (my_server_handle == NULL) {
        my_server_handle = setup_server();
        ESP_LOGI(TAG, "Custom HTTP server started for main page.");
    }

    // Print the new IP address
    esp_netif_ip_info_t ip_info;
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        ESP_LOGI(TAG, "ESP32 STA IP: " IPSTR, IP2STR(&ip_info.ip));
    }
}


// The retry logic for WiFi reconnection and fallback to provisioning
void wifi_manager_sta_disconnected(void *pvParameter) {
    wifi_manager_retry_count++;
    if (wifi_manager_retry_count >= WIFI_MANAGER_MAX_RETRIES) {
        ESP_LOGW(TAG, "Max WiFi retries reached, restarting provisioning.");

        // Stop your custom server if running
        if (my_server_handle) {
            httpd_stop(my_server_handle);
            my_server_handle = NULL;
        }

        wifi_manager_destroy();
        vTaskDelay(pdMS_TO_TICKS(500));
        wifi_manager_start();
        wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
        wifi_manager_set_callback(WM_EVENT_STA_DISCONNECTED, &wifi_manager_sta_disconnected);

        // Start wifi_manager HTTP server again (provisioning page)
        extern void http_app_start(bool lru_purge_enable);
        http_app_start(true);

        wifi_manager_retry_count = 0;
    }
}


void app_main()
{
    // Only erase NVS if you want to force a clean start (not recommended for production)
    // ESP_ERROR_CHECK(nvs_flash_erase());
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());

    // Start wifi manager
    wifi_manager_start();

    // Register callback for when WiFi is connected or
    wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
    wifi_manager_set_callback(WM_EVENT_STA_DISCONNECTED, &wifi_manager_sta_disconnected);


    // GPIO initialization and web server setup can be done after WiFi is up,
    // or you can keep them here if you want them always available.
    gpio_reset_pin(LED_PIN2);
    gpio_set_direction(LED_PIN2, GPIO_MODE_OUTPUT); 
    gpio_reset_pin(LED_PIN3); 
    gpio_set_direction(LED_PIN3, GPIO_MODE_OUTPUT); 
    gpio_reset_pin(LED_PIN4); 
    gpio_set_direction(LED_PIN4, GPIO_MODE_OUTPUT); 
    gpio_reset_pin(LED_PIN15); 
    gpio_set_direction(LED_PIN15, GPIO_MODE_OUTPUT); 
    led_state = 0;
    ESP_LOGI(TAG, "LEDs initialised to 0 ... ...\n");
}






// In your app_main or wifi_manager setup, register this callback for STA disconnects:


// NOTE: As of esp32-wifi-manager v0.0.4, only the following events are supported for callbacks:
//   - WM_EVENT_STA_GOT_IP
//   - WM_EVENT_STA_DISCONNECTED
//   - WM_EVENT_AP_STA_CONNECTED
//   - WM_EVENT_AP_STA_DISCONNECTED
//   - WM_ORDER_START_WIFI_SCAN
//   - WM_ORDER_LOAD_AND_RESTORE_STA

// Therefore, WM_EVENT_STA_DISCONNECTED *is* supported and you can use it as shown above.
