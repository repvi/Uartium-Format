#include <stdio.h>
#include <stdint.h>
#include "uartium.h"

// Mock timestamp function for demonstration
static uint32_t g_timestamp = 1000;
uint32_t mock_time_fn(void) {
    return g_timestamp++;
}

int main(void)
{
    char buffer[512];
    
    printf("=== Demo 1: Basic struct data with event types ===\n");
    uartium_config_t cfg1 = { (uint8_t*)buffer, sizeof(buffer), NULL, NULL };
    uartium_init(&cfg1);
    
    typedef struct { uint32_t temp; int pressure; float humidity; char *location; } sensor_t;
    sensor_t s1 = { 25, 1013, 45.5f, "Room A" };
    sensor_t s2 = { 82, 980, 90.2f, "Boiler" };
    /* Additional sample to demonstrate multi-word string handling */
    sensor_t s_sna = { 0, 0, 0.0f, "sna such though" };
    
    const uartium_field_t fields[] = {
        UARTIUM_FIELD_UINT(sensor_t, temp),
        UARTIUM_FIELD_INT(sensor_t, pressure),
        UARTIUM_FIELD_FLOAT(sensor_t, humidity),
        UARTIUM_FIELD_STRING(sensor_t, location),
    };
    
    uartium_buffer_struct_fields(&s1, fields, 4, UARTIUM_EVENT_INFO, "Normal reading");
    uartium_buffer_struct_fields(&s2, fields, 4, UARTIUM_EVENT_WARNING, "High temperature detected");
    uartium_buffer_struct_fields(&s_sna, fields, 4, UARTIUM_EVENT_INFO, "Show string sna such though");
    
    const uint8_t *buf;
    size_t len;
    uartium_get_buffer(&buf, &len);
    printf("%s\n\n", (const char*)buf);
    
    uartium_deinit();
    
    printf("=== Demo 2: With timestamps ===\n");
    uartium_config_t cfg2 = { (uint8_t*)buffer, sizeof(buffer), NULL, mock_time_fn };
    uartium_init(&cfg2);
    
    sensor_t s3 = { 22, 1015, 50.0f, "Office" };
    sensor_t s4 = { 95, 950, 98.5f, "Engine" };
    
    uartium_buffer_message(UARTIUM_EVENT_INFO, "System startup");
    uartium_buffer_struct_fields(&s3, fields, 4, UARTIUM_EVENT_INFO, NULL);
    /* Also show the multi-word string entry with timestamps enabled */
    uartium_buffer_struct_fields(&s_sna, fields, 4, UARTIUM_EVENT_INFO, "Show string sna such though");
    uartium_buffer_struct_fields(&s4, fields, 4, UARTIUM_EVENT_ERROR, "Critical overheat");
    uartium_buffer_message(UARTIUM_EVENT_DEBUG, "Debug checkpoint");
    
    uartium_get_buffer(&buf, &len);
    printf("%s\n\n", (const char*)buf);
    
    uartium_deinit();
    
    printf("=== Demo 3: Mixed message types ===\n");
    uartium_config_t cfg3 = { (uint8_t*)buffer, sizeof(buffer), NULL, mock_time_fn };
    uartium_init(&cfg3);
    
    uartium_log_message(UARTIUM_EVENT_INFO, "Starting diagnostics");
    uartium_log_struct_fields(&s1, fields, 4, UARTIUM_EVENT_DEBUG, "Baseline measurement");
    uartium_log_message(UARTIUM_EVENT_WARNING, "Threshold approaching");
    uartium_log_struct_fields(&s2, fields, 4, UARTIUM_EVENT_ERROR, "Limit exceeded");
    
    uartium_deinit();
    
    printf("\n=== All demos completed successfully ===\n");
    return 0;
}
