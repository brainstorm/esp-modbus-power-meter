#include <esp_rmaker_utils.h>
#include <string.h>

#include "sdkconfig.h"
#include "app_pvoutput_org.h"
#include "app_wifi.h"
#include "app_time.h"
#include "app_modbus.h"

#define MAX_HTTP_RECV_BUFFER    512
#define MAX_HTTP_OUTPUT_BUFFER  8192

static const char *TAG = "app_pvoutput";

#define PVOUTPUT_ADD_STATUS_STR_MAX_LEN 60

// extern const uint8_t server_root_cert_pem_start[] asm("_binary_pvoutput_server_pem_start");
// extern const uint8_t server_root_cert_pem_end[]   asm("_binary_pvoutput_server_pem_end");

char* g_pvoutput_query_string;
extern float g_current_volts;
extern float g_current_watts;

// static const char REQUEST[] = "POST pvoutput.org HTTP/1.1\r\n"
//                               "Host: pvoutput.org\r\n"
//                               "User-Agent: esp-idf/1.0 esp32\r\n"
//                               "\r\n";

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        #if ESP_IDF_VERSION_MAJOR >= 5
        case HTTP_EVENT_REDIRECT:
            break;
        #endif
        case HTTP_EVENT_ERROR:
            ESP_LOGI(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER");
            printf("%.*s", evt->data_len, (char*)evt->data);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            if (!esp_http_client_is_chunked_response(evt->client)) {
                printf("%.*s", evt->data_len, (char*)evt->data);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGI(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
    }
    return ESP_OK;
}

char* build_pvoutput_query_string(char* date, char* time, float watts, float volts) {
   char dst[PVOUTPUT_ADD_STATUS_STR_MAX_LEN]; // i.e: addstatus.jsp?d=20220328&t=21:56&v2=1&v6=230

   snprintf(dst,
            PVOUTPUT_ADD_STATUS_STR_MAX_LEN,
            "d=%s&t=%s&v2=%f&v6=%f",
            date,
            time,
            watts, 
            volts);

   char* query_str = calloc(1, PVOUTPUT_ADD_STATUS_STR_MAX_LEN);
   strncpy(query_str, dst, PVOUTPUT_ADD_STATUS_STR_MAX_LEN);

   return query_str;
}

void app_pvoutput_init() {
    ESP_LOGI(TAG, "Current date is: %s", get_pvoutput_fmt_date()); 
    ESP_LOGI(TAG, "Current time is: %s", get_pvoutput_fmt_time());
}

void pvoutput_update()
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};

    // Hack, hack, hack, use queues!!!
    // Do not report to pvoutput if those two have not been touched yet, it'd skew the plots
    if (g_current_volts == -0.1) return;
    if (g_current_watts == -0.1) return;

    // Bail out early if SNTP is not set. Things will not work properly if system time is not set properly
    // i.e:
    // PVOutput does not allow status data submissions older than 14 days...
    // ... and 1970 happened a long time ago ;)
    if (esp_rmaker_time_check() != true) return;


    g_pvoutput_query_string = build_pvoutput_query_string(get_pvoutput_fmt_date(),
                                                          get_pvoutput_fmt_time(),
                                                          g_current_watts,  // Watts
                                                          g_current_volts); // Volts

    esp_http_client_config_t config = {
        .host = "pvoutput.org",
        .path = "/service/r2/addstatus.jsp",
        .method = HTTP_METHOD_POST,
        .query = g_pvoutput_query_string,
        .event_handler = _http_event_handler,
        .user_data = local_response_buffer,        // Pass address of local buffer to get response
        .disable_auto_redirect = true,
        //.use_global_ca_store = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "X-Pvoutput-Apikey", CONFIG_PVOUTPUT_ORG_API_KEY);
    esp_http_client_set_header(client, "X-Pvoutput-SystemId", CONFIG_PVOUTPUT_ORG_SYSTEM_ID);

    ESP_LOGI(TAG, "Sending power data to: https://%s%s%s", config.host, config.path, g_pvoutput_query_string);
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        #if ESP_IDF_VERSION_MAJOR >= 5
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRIx64"",
        #else
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %x",
        #endif
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));

        xTaskNotifyGive(modbus_task);
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }
}