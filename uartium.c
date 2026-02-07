#include "uartium.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define UARTIUM_MESSAGE_TYPE_DATA    "[DATA]"
#define UARTIUM_MESSAGE_TYPE_ERROR   "[ERROR]"
#define UARTIUM_MESSAGE_TYPE_DEBUG   "[DEBUG]"
#define UARTIUM_MESSAGE_TYPE_WARNING "[WARNING]"
#define UARTIUM_MESSAGE_TYPE_INFO    "[INFO]"

typedef struct {
    uint8_t *buffer;
    size_t buffer_size;
    size_t buffer_index;
    uartium_write_fn_t write_fn;

    bool initialized;
} uartium_ctx_t;

static uartium_ctx_t s_uartium_ctx = {0};

static const char *uartium_event_type_strings[] = {
    UARTIUM_MESSAGE_TYPE_DATA,
    UARTIUM_MESSAGE_TYPE_ERROR,
    UARTIUM_MESSAGE_TYPE_DEBUG,
    UARTIUM_MESSAGE_TYPE_WARNING,
    UARTIUM_MESSAGE_TYPE_INFO
};

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
    s_uartium_ctx.initialized    = true;
    printf("Uartium initialized!\n");
    return UARTIUM_STATUS_OK;
}

uartium_status_t uartium_deinit()
{
    if (!s_uartium_ctx.initialized) {
        return UARTIUM_STATUS_ERROR;
    }

    s_uartium_ctx.initialized = false;
    s_uartium_ctx.buffer = NULL;
    s_uartium_ctx.buffer_size = 0;
    s_uartium_ctx.buffer_index = 0;
    s_uartium_ctx.write_fn = NULL;
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

uartium_status_t uartium_msg_helper(uartium_ctx_t *const ctx, uartium_event_type_t type, const char* msg) 
{
    const char *type_str     = uartium_event_type_strings[type];
    size_t type_len          = strlen(type_str);
    size_t msg_len           = strlen(msg);
    
    uartium_status_t status = new_line_if_needed(ctx);
    if (status != UARTIUM_STATUS_OK) {
        ctx->buffer[ctx->buffer_index] = '\0'; // Null-terminate the buffer on error
        return status;
    }

    status = uartium_append_string(ctx, "%s %s\n", type_str, msg, &ctx->buffer_index);
    if (status != UARTIUM_STATUS_OK) {
        ctx->buffer[ctx->buffer_index] = '\0'; // Null-terminate the buffer on error
        return status;
    }

    return UARTIUM_STATUS_OK;
}

uartium_status_t uartium_buffer_message(uartium_event_type_t type, const char* msg) 
{
    uartium_ctx_t *const ctx = &s_uartium_ctx;
    if (!ctx->initialized || valid_event_type(type) == false || msg == NULL) {
        return UARTIUM_STATUS_ERROR;
    }

    return uartium_msg_helper(ctx, type, msg);
}

uartium_status_t uartium_log_message(uartium_event_type_t type, const char* msg) 
{
    uartium_ctx_t *const ctx = &s_uartium_ctx;

    if (!ctx->initialized || valid_event_type(type) == false || msg == NULL) {
        return UARTIUM_STATUS_ERROR;
    }

    uartium_status_t status = uartium_msg_helper(ctx, type, msg);
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
                        size_t field_count)
{
    size_t i                 = 0;
    size_t retries           = 0;
    size_t iter              = 0;
    
    new_line_if_needed(ctx);
    iter = ctx->buffer_index;
    
    while (i < field_count) {
        const uartium_field_t* const f = &fields[i];
        const uint8_t* base            = (const uint8_t*)data + f->offset;
        uartium_status_t status        = UARTIUM_STATUS_OK;

        switch (f->type) {
            case UARTIUM_F_FLOAT:
                status = uartium_append_float(ctx, "%s=%.3f", f->name, *(float*)base, &iter);
                break;

            case UARTIUM_F_UINT:
                status = uartium_append_uint(ctx, "%s=%u", f->name, *(uint32_t*)base, &iter);
                break;

            case UARTIUM_F_INT:
                status = uartium_append_int(ctx, "%s=%d", f->name, *(int*)base, &iter);
                break;

            case UARTIUM_F_STRING:
                status = uartium_append_string(ctx, "%s=%s", f->name, *(char**)base, &iter);
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
                           size_t field_count) 
{
    uartium_ctx_t *const ctx = &s_uartium_ctx;
    if (!ctx->initialized || data == NULL || fields == NULL || field_count == 0) {
        return UARTIUM_STATUS_ERROR;
    }

    return uartium_log_struct_fields_helper(ctx, data, fields, field_count);
}

uartium_status_t uartium_log_struct_fields(const void* data,
                        const uartium_field_t* const fields,
                        size_t field_count)
{
    uartium_ctx_t *const ctx = &s_uartium_ctx;
    if (!ctx->initialized || data == NULL || fields == NULL || field_count == 0) {
        return UARTIUM_STATUS_ERROR;
    }

    uartium_status_t status = uartium_log_struct_fields_helper(ctx, data, fields, field_count);
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

    *buffer                     = ctx->buffer;
    *buffer_size                = ctx->buffer_index;
    return UARTIUM_STATUS_OK;
}
