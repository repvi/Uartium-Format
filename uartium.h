#ifndef UARTIUM_H
#define UARTIUM_H

#include <stddef.h>
#include <stdint.h>

typedef void(*uartium_write_fn_t)(const char* buffer, size_t len);

typedef struct uartium_config_t
{
    uint8_t *buffer;
    size_t buffer_size;
    uartium_write_fn_t write_fn; // used for printing log messages, if NULL, defaults to printf
} uartium_config_t;

typedef enum uartium_event_type_t
{
    UARTIUM_EVENT_INFO = 0,
    UARTIUM_EVENT_WARNING,
    UARTIUM_EVENT_DEBUG,
    UARTIUM_EVENT_ERROR
} uartium_event_type_t;

typedef enum uartium_status_t
{
    UARTIUM_STATUS_OK = 0,
    UARTIUM_STATUS_ERROR,
    UARTIUM_STATUS_BUFFER_OVERFLOW
} uartium_status_t;

typedef enum uartium_field_type_t
{ 
    UARTIUM_F_UINT, 
    UARTIUM_F_INT, 
    UARTIUM_F_FLOAT, 
    UARTIUM_F_STRING 
} uartium_field_type_t;

typedef struct 
{ 
    const char* name; 
    size_t offset; 
    uartium_field_type_t type; 
} uartium_field_t;

#define UARTIUM_FIELD_UINT(struct_type, field) \
    { #field, offsetof(struct_type, field), UARTIUM_F_UINT }

#define UARTIUM_FIELD_FLOAT(struct_type, field) \
    { #field, offsetof(struct_type, field), UARTIUM_F_FLOAT }

#define UARTIUM_FIELD_INT(struct_type, field) \
    { #field, offsetof(struct_type, field), UARTIUM_F_INT }

#define UARTIUM_FIELD_STRING(struct_type, field) \
    { #field, offsetof(struct_type, field), UARTIUM_F_STRING }


#ifdef __cplusplus
extern "C" {
#endif

uartium_status_t uartium_init(const uartium_config_t* const config);

uartium_status_t uartium_deinit();

uartium_status_t uartium_buffer_message(uartium_event_type_t type, const char* msg);

uartium_status_t uartium_log_message(uartium_event_type_t type, const char* msg);

uartium_status_t uartium_flush();

uartium_status_t uartium_buffer_struct_fields(const void* data,
                           const uartium_field_t* const fields,
                           size_t field_count);

uartium_status_t uartium_log_struct_fields(const void* data,
                        const uartium_field_t* const fields,
                        size_t field_count);

uartium_status_t uartium_get_buffer(const uint8_t** buffer, size_t* buffer_size);


#ifdef __cplusplus
}
#endif

#endif // UARTIUM_H