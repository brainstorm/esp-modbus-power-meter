#include <esp_rmaker_utils.h>

#include <string.h>

#include "sdkconfig.h"
#include "app_pvoutput_org.h"
#include "app_wifi.h"
#include "app_time.h"
#include "app_modbus.h"

#include <esp_err.h>

#define MAX_HTTP_RECV_BUFFER    512
#define MAX_HTTP_OUTPUT_BUFFER  8192

static const char *TAG = "app_pvoutput";

#define PVOUTPUT_ADD_STATUS_STR_MAX_LEN 60

// extern const uint8_t server_root_cert_pem_start[] asm("_binary_pvoutput_server_pem_start");
// extern const uint8_t server_root_cert_pem_end[]   asm("_binary_pvoutput_server_pem_end");

char* g_pvoutput_query_string;

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
void print_current_datetime() {
    ESP_LOGI(TAG, "Current date is: %s", get_pvoutput_fmt_date()); 
    ESP_LOGI(TAG, "Current time is: %s", get_pvoutput_fmt_time());
}

int app_pvoutput_init() {
    //print_current_datetime();
    xTaskCreate(pvoutput_update, "pvoutput_task", 8192, NULL, 5, &pvoutput_task);
    
    // The task above should never end, if it does, that's a fail :-!
    return ESP_FAIL;
}

void pvoutput_update()
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER] = {0};
    uint32_t ulNotifiedValue;

    while(1) {
        // Block here if SNTP is not set. Things will not work properly if system time is not set properly
        // i.e:
        // PVOutput does not allow status data submissions older than 14 days...
        // ... and 1970 happened a long time ago ;)

        while (esp_rmaker_time_check() != true) {
            esp_rmaker_time_wait_for_sync(pdMS_TO_TICKS(10000));
            //print_current_datetime();
            vTaskDelay(pdMS_TO_TICKS(1000)); 
        }

        xTaskNotifyWait(0, ULONG_MAX, &ulNotifiedValue, portMAX_DELAY);

        // XXX: Find a better way for obscure Watts/Volts index in mb_readings
        g_pvoutput_query_string = build_pvoutput_query_string(get_pvoutput_fmt_date(),
                                                            get_pvoutput_fmt_time(),
                                                            mb_readings[3].value,  // Watts
                                                            mb_readings[6].value); // Volts

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
        // XXX: Do not setup key if it starts with "<" (not setup)
        esp_http_client_set_header(client, "X-Pvoutput-Apikey", CONFIG_PVOUTPUT_ORG_API_KEY);
        esp_http_client_set_header(client, "X-Pvoutput-SystemId", CONFIG_PVOUTPUT_ORG_SYSTEM_ID);

        ESP_LOGI(TAG, "Sending power data to: https://%s%s%s", config.host, config.path, g_pvoutput_query_string);
        // XXX: Re-enable when I'm sure rate limiting is right
        //esp_err_t err = esp_http_client_perform(client);
        // if (err == ESP_OK) {
        //     #if ESP_IDF_VERSION_MAJOR >= 5
        //     ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %"PRIx64"",
        //     #else
        //     ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %x",
        //     #endif
        //             esp_http_client_get_status_code(client),
        //             esp_http_client_get_content_length(client));
        // } else {
        //     ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        // }

        // Done with reading modbus values and reporting them
        ESP_LOGI(TAG, "Notifying modbus_task that we are done with one pvoutput data submission...\n");
        xTaskNotifyGive(modbus_task);
    }
}