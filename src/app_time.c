#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stddef.h>
#include <string.h>

#include <esp_log.h>

#include "app_time.h"

const char* TAG="app_time";

// i.e: 23:54
#define PVOUTOUT_TIME_STR_MAX_LEN 6
// i.e: 20210211
#define PVOUTPUT_DATE_STR_MAX_LEN 9

char* get_pvoutput_fmt_time() {
    time_t now;
    char strftime_buf[PVOUTOUT_TIME_STR_MAX_LEN];
    char* final_date_string;
    struct tm timeinfo;

    time(&now);
    // Set timezone to Lisbon Time
    setenv("TZ", "Europe/Lisbon", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%H:%M", &timeinfo);
    final_date_string = calloc(1, sizeof(strftime_buf));
    strncat(final_date_string, strftime_buf, PVOUTOUT_TIME_STR_MAX_LEN - 1);

    return final_date_string;
}

char* get_pvoutput_fmt_date() {
    time_t now;
    char strftime_buf[PVOUTPUT_DATE_STR_MAX_LEN];
    char* final_date_string;
    struct tm timeinfo;

    time(&now);
    // Set timezone to Lisbon Time
    setenv("TZ", "Europe/Lisbon", 1);
    tzset();

    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%Y%m%d", &timeinfo);
    final_date_string = calloc(1, sizeof(strftime_buf));
    strncat(final_date_string, strftime_buf, PVOUTPUT_DATE_STR_MAX_LEN - 1);

    return final_date_string;
}