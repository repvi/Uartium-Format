#include "uartium.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* Public message type definitions */
#define UARTIUM_MESSAGE_TYPE_ERROR   "[ERROR]"
#define UARTIUM_MESSAGE_TYPE_DEBUG   "[DEBUG]"
#define UARTIUM_MESSAGE_TYPE_WARNING "[WARNING]"
#define UARTIUM_MESSAGE_TYPE_INFO    "[INFO]"

/* Internal context and helper functions */
#define UARTIUM_MESSAGE_TYPE_UNKNOWN "[UNKNOWN]"

typedef struct {
    uint8_t *buffer;
    size_t buffer_size;
    size_t buffer_index;
    uartium_write_fn_t write_fn;
    uartium_time_fn_t time_fn;
    bool initialized;
} uartium_ctx_t;

static uartium_ctx_t s_uartium_ctx = {0};

static const char *uartium_get_internal_header(uint32_t id) 
{
    switch((uartium_event_type_t)id) {
        case UARTIUM_EVENT_INFO:
            return UARTIUM_MESSAGE_TYPE_INFO;
        case UARTIUM_EVENT_WARNING:
            return UARTIUM_MESSAGE_TYPE_WARNING;
        case UARTIUM_EVENT_DEBUG:
            return UARTIUM_MESSAGE_TYPE_DEBUG;
        case UARTIUM_EVENT_ERROR:
            return UARTIUM_MESSAGE_TYPE_ERROR;
        default:
            break;
    }
    
    return UARTIUM_MESSAGE_TYPE_UNKNOWN;
}

static inline uartium_status_t uartium_append_string(uartium_ctx_t *const ctx, const char* format, const char *name, const char* data, size_t *offset);
void uartium_flush_internal(uartium_ctx_t *const ctx);

uartium_status_t uartium_init(const uartium_config_t* const config)
{
    if (s_uartium_ctx.initialized || config == NULL || config->buffer == NULL || config->buffer_size == 0) {
        return UARTIUM_STATUS_ERROR;
    }

    s_uartium_ctx.buffer         = config->buffer;
    s_uartium_ctx.buffer_size    = config->buffer_size;
    s_uartium_ctx.buffer_index   = 0;
    s_uartium_ctx.buffer[0]      = '\0'; // Initialize the buffer with a null terminator
    s_uartium_ctx.write_fn       = config->write_fn;
    s_uartium_ctx.time_fn        = config->time_fn;
    s_uartium_ctx.initialized    = true;
    printf("Uartium initialized!\n");
    return UARTIUM_STATUS_OK;
}

uartium_status_t uartium_deinit()
{
    if (!s_uartium_ctx.initialized) {
        return UARTIUM_STATUS_ERROR;
    }

    s_uartium_ctx.initialized     = false;
    s_uartium_ctx.buffer          = NULL;
    s_uartium_ctx.buffer_size     = 0;
    s_uartium_ctx.buffer_index    = 0;
    s_uartium_ctx.write_fn        = NULL;
    s_uartium_ctx.time_fn         = NULL;
    printf("Uartium deinitialized!\n");
    return UARTIUM_STATUS_OK;
}

static inline bool valid_event_type(uartium_event_type_t type) 
{
    switch(type) {
        case UARTIUM_EVENT_INFO:
        case UARTIUM_EVENT_WARNING:
        case UARTIUM_EVENT_DEBUG:
        case UARTIUM_EVENT_ERROR:
            return true;
        default:
            return false;
    }
}

static inline uartium_status_t new_line_if_needed(uartium_ctx_t *const ctx)
{
    if (ctx->buffer_index != 0) {
        if (ctx->buffer_index < ctx->buffer_size - 1) {
            ctx->buffer[ctx->buffer_index] = '\n';
            ctx->buffer_index++;
        }
        else {
            return UARTIUM_STATUS_BUFFER_OVERFLOW;
        }
    }
    return UARTIUM_STATUS_OK;
}

static bool has_text(const char *s) 
{
    return (s != NULL && s[0] != '\0');
}

static bool is_valid_message(const char *msg) {
    if (msg == NULL) {
        return false;
    }

    for (size_t i = 0; msg[i] != '\0'; i++) {
        if (msg[i] == '\n' || msg[i] == '\r') {
            return false; // forbidden control characters
        }
    }

    return true;
}

static uartium_status_t uartium_msg_internal(uartium_ctx_t *const ctx, uint32_t id, const char* msg) 
{
    const char *type_str     = uartium_get_internal_header(id);
    size_t type_len          = strlen(type_str);
    size_t msg_len           = strlen(msg);
    
    uartium_status_t status = new_line_if_needed(ctx);
    if (status != UARTIUM_STATUS_OK) {
        ctx->buffer[ctx->buffer_index] = '\0'; // Null-terminate the buffer on error
        return status;
    }

    // Add event type and message
    if (has_text(msg)) {
        status = uartium_append_string(ctx, "%s %s", type_str, msg, &ctx->buffer_index);
    }
    else {
        status = uartium_append_string(ctx, "%s%s", type_str, " ", &ctx->buffer_index);
    }

    // Add timestamp if time_fn is provided
    if (ctx->time_fn) {
        uint32_t timestamp = ctx->time_fn();
        uint8_t *offset_ptr = ctx->buffer + ctx->buffer_index;
        size_t buffer_count = ctx->buffer_size - ctx->buffer_index;
        int n = snprintf((char*)offset_ptr, buffer_count, ":t=%u ", timestamp);
        if (n < 0 || (size_t)n >= buffer_count) {
            ctx->buffer[ctx->buffer_index] = '\0';
            return UARTIUM_STATUS_BUFFER_OVERFLOW;
        }
        ctx->buffer_index += n;
    }

    return status;
}

uartium_status_t uartium_buffer_message(uartium_event_type_t type, const char* msg) 
{
    uartium_ctx_t *const ctx = &s_uartium_ctx;
    if (!ctx->initialized || !valid_event_type(type) || !msg || !is_valid_message(msg)) {
        return UARTIUM_STATUS_ERROR;
    }

    return uartium_msg_internal(ctx, (uint32_t)type, msg);
}

uartium_status_t uartium_log_message(uartium_event_type_t type, const char* msg) 
{
    uartium_ctx_t *const ctx = &s_uartium_ctx;

    if (!ctx->initialized || !valid_event_type(type) || !msg || !is_valid_message(msg)) {
        return UARTIUM_STATUS_ERROR;
    }

    uartium_status_t status = uartium_msg_internal(ctx, (uint32_t)type, msg);
    if (status == UARTIUM_STATUS_OK) {
        uartium_flush_internal(ctx);
    }
    return status;
}

void uartium_flush_internal(uartium_ctx_t *const ctx) 
{
    if (ctx->write_fn) {
        ctx->write_fn(ctx->buffer, ctx->buffer_index);
    }
    else {
        if (ctx->buffer_index == 0) {
            printf("Flushing Uartium buffer: <empty>\n");
        } else {
            printf("Flushing Uartium buffer (index=%zu): %s\n", ctx->buffer_index, (char*)(ctx->buffer));
        }
    }
    ctx->buffer[0] = '\0'; // Clear the buffer after flushing
    ctx->buffer_index = 0;
}

uartium_status_t uartium_flush()
{
    uartium_ctx_t *const ctx = &s_uartium_ctx;
    if (!ctx->initialized || ctx->buffer == NULL || ctx->buffer_size == 0) {
        return UARTIUM_STATUS_ERROR;
    }

    uartium_flush_internal(ctx);
    return UARTIUM_STATUS_OK;
}

static inline uartium_status_t uartium_append_float(uartium_ctx_t *const ctx, const char* format, const char *name, float data, size_t *offset)
{
    uint8_t *const offset_ptr    = ctx->buffer + *offset;
    const size_t buffer_count    = ctx->buffer_size - *offset;
    int n = snprintf(offset_ptr, buffer_count, format, name, data);
    if (n < 0 || n >= buffer_count) {
        ctx->buffer[ctx->buffer_index] = '\0'; // Null-terminate the buffer on error
        return UARTIUM_STATUS_BUFFER_OVERFLOW;
    }
    *offset += n;
    return UARTIUM_STATUS_OK;
}

static inline uartium_status_t uartium_append_uint(uartium_ctx_t *const ctx, const char* format, const char *name, uint32_t data, size_t *offset)
{
    uint8_t *const offset_ptr    = ctx->buffer + *offset;
    const size_t buffer_count    = ctx->buffer_size - *offset;
    int n = snprintf(offset_ptr, buffer_count, format, name, data);
    if (n < 0 || n >= buffer_count) {
        ctx->buffer[ctx->buffer_index] = '\0'; // Null-terminate the buffer on error
        return UARTIUM_STATUS_BUFFER_OVERFLOW;
    }
    *offset += n;
    return UARTIUM_STATUS_OK;
}

static inline uartium_status_t uartium_append_int(uartium_ctx_t *const ctx, const char* format, const char *name, int data, size_t *offset)
{
    uint8_t *const offset_ptr    = ctx->buffer + *offset;
    const size_t buffer_count    = ctx->buffer_size - *offset;
    int n = snprintf(offset_ptr, buffer_count, format, name, data);
    if (n < 0 || n >= buffer_count) {
        ctx->buffer[ctx->buffer_index] = '\0'; // Null-terminate the buffer on error
        return UARTIUM_STATUS_BUFFER_OVERFLOW;
    }
    *offset += n;
    return UARTIUM_STATUS_OK;
}

static inline uartium_status_t uartium_append_string(uartium_ctx_t *const ctx, const char* format, const char *name, const char* data, size_t *offset)
{
    uint8_t *const offset_ptr    = ctx->buffer + *offset;
    const size_t buffer_count    = ctx->buffer_size - *offset;
    int n = snprintf(offset_ptr, buffer_count, format, name, data);
    if (n < 0 || n >= buffer_count) {
        ctx->buffer[ctx->buffer_index] = '\0'; // Null-terminate the buffer on error
        return UARTIUM_STATUS_BUFFER_OVERFLOW;
    }
    *offset += n;
    return UARTIUM_STATUS_OK;
}

static inline uartium_status_t uartium_append_separator(uartium_ctx_t *const ctx, size_t *offset)
{
    uint8_t *const offset_ptr    = ctx->buffer + *offset;
    const size_t buffer_count    = ctx->buffer_size - *offset;

    size_t avail = ctx->buffer_size - *offset;
    if (avail < 2) {
        ctx->buffer[ctx->buffer_index] = '\0'; // Null-terminate the buffer on error
        return UARTIUM_STATUS_BUFFER_OVERFLOW;
    }
    ctx->buffer[*offset] = ' ';
    (*offset)++;
    return UARTIUM_STATUS_OK;
}

static uartium_status_t flush_and_retry(uartium_ctx_t *const ctx, size_t *iter, size_t *i, size_t *retries)
{
    uartium_flush_internal(ctx);
    /* After a flush, continue writing at buffer_index + 1 to preserve the
       library's convention of leaving room for the leading character. */
    *iter = ctx->buffer_index + 1; // Reset index after flushing
    /* Retry the current field: decrement i if possible, otherwise keep at 0. */
    if (*i > 0) {
        (*i)--;
    } else {
        *i = 0;
    }
    (*retries)++;
    if (*retries > 1) {
        return UARTIUM_STATUS_BUFFER_OVERFLOW;
    }
    return UARTIUM_STATUS_OK;
}

static uartium_status_t uartium_log_struct_fields_helper(
                        uartium_ctx_t *const ctx,
                        const void* data,
                        const uartium_field_t* const fields,
                        size_t field_count,
                        uartium_event_type_t event_type,
                        const char *msg)
{
    size_t i                 = 0;
    size_t retries           = 0;
    size_t iter              = 0;
    
    // Don't call new_line_if_needed here - uartium_msg_internal will do it
    iter = ctx->buffer_index;
    uartium_msg_internal(ctx, event_type, msg);
    iter = ctx->buffer_index;  // Update iter to point after the message header
    
    // Add separator between message and fields if message was provided
    if (msg && msg[0] != '\0') {
        uartium_status_t sep_status = uartium_append_separator(ctx, &iter);
        if (sep_status == UARTIUM_STATUS_BUFFER_OVERFLOW) {
            ctx->buffer[ctx->buffer_index] = '\0';
            return UARTIUM_STATUS_BUFFER_OVERFLOW;
        }
    }
    
    while (i < field_count) {
        const uartium_field_t* const f = &fields[i];
        const uint8_t* base            = (const uint8_t*)data + f->offset;
        uartium_status_t status        = UARTIUM_STATUS_OK;

        switch (f->type) {
            case UARTIUM_F_FLOAT:
                status = uartium_append_float(ctx, "%s:f=%.3f", f->name, *(float*)base, &iter);
                break;

            case UARTIUM_F_UINT:
                status = uartium_append_uint(ctx, "%s:u=%u", f->name, *(uint32_t*)base, &iter);
                break;

            case UARTIUM_F_INT:
                status = uartium_append_int(ctx, "%s:i=%d", f->name, *(int*)base, &iter);
                break;

            case UARTIUM_F_STRING:
                status = uartium_append_string(ctx, "%s:s=\"%s\"", f->name, *(char**)base, &iter);
                break;

            default:
                fprintf(stderr, "Error: Unknown field type for field '%s'.\n", f->name);
                return UARTIUM_STATUS_ERROR;
        }

        if (status == UARTIUM_STATUS_BUFFER_OVERFLOW) {
            if (flush_and_retry(ctx, &iter, &i, &retries) == UARTIUM_STATUS_BUFFER_OVERFLOW) {
                return UARTIUM_STATUS_BUFFER_OVERFLOW;
            }
            continue;
        }
        
        if (i != field_count - 1) {
            uartium_status_t sep_status = uartium_append_separator(ctx, &iter);
            if (sep_status == UARTIUM_STATUS_BUFFER_OVERFLOW) {
                if (flush_and_retry(ctx, &iter, &i, &retries) == UARTIUM_STATUS_BUFFER_OVERFLOW) {
                    return UARTIUM_STATUS_BUFFER_OVERFLOW;
                }
                continue;
            }
        }

        i++;
    }

    ctx->buffer[iter] = '\0'; // Null-terminate the buffer
    ctx->buffer_index = iter; // Update the buffer index after processing all fields
    return UARTIUM_STATUS_OK;
}

uartium_status_t uartium_buffer_struct_fields(const void* data,
                           const uartium_field_t* const fields,
                           size_t field_count,
                           uartium_event_type_t event_type,
                           const char *msg) 
{
    uartium_ctx_t *const ctx = &s_uartium_ctx;
    if (!ctx->initialized || !data || !fields || field_count == 0 || !valid_event_type(event_type)) {
        return UARTIUM_STATUS_ERROR;
    }

    if (!msg) {
        msg = "";
    } else if (!is_valid_message(msg)) {
        return UARTIUM_STATUS_ERROR;
    }

    return uartium_log_struct_fields_helper(ctx, data, fields, field_count, event_type, msg);
}

uartium_status_t uartium_log_struct_fields(const void* data,
                        const uartium_field_t* const fields,
                        size_t field_count,
                        uartium_event_type_t event_type,
                        const char *msg)
{
    uartium_ctx_t *const ctx = &s_uartium_ctx;
    if (!ctx->initialized || !data || !fields || field_count == 0 || !valid_event_type(event_type)) {
        return UARTIUM_STATUS_ERROR;
    }

    if (!msg) {
        msg = "";
    } else if (!is_valid_message(msg)) {
        return UARTIUM_STATUS_ERROR;
    }

    uartium_status_t status = uartium_log_struct_fields_helper(ctx, data, fields, field_count, event_type, msg);
    if (status == UARTIUM_STATUS_OK) {
        uartium_flush_internal(ctx);
    }
    return status;
}

uartium_status_t uartium_get_buffer(const uint8_t** buffer, size_t* buffer_size)
{
    uartium_ctx_t *const ctx = &s_uartium_ctx;

    if (!ctx->initialized || buffer == NULL || buffer_size == NULL) {
        return UARTIUM_STATUS_ERROR;
    }

    *buffer         = ctx->buffer;
    *buffer_size    = ctx->buffer_index;
    return UARTIUM_STATUS_OK;
}
