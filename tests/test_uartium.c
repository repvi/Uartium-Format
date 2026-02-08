#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "uartium.h"

#define ASSERT(cond, msg) do { if (!(cond)) { fprintf(stderr, "ASSERT FAILED: %s\n", msg); return 1; } } while(0)

static int test_format_and_flush(void)
{
    char buffer[256];
    uartium_config_t cfg = { (uint8_t*)buffer, sizeof(buffer), NULL };

    ASSERT(uartium_init(&cfg) == UARTIUM_STATUS_OK, "uartium_init failed");

    typedef struct {
        uint32_t a;
        int b;
        float c;
        char* s;
    } test_t;

    test_t t = { 123u, -5, 1.2345f, "hello" };

    const uartium_field_t fields[] = {
        UARTIUM_FIELD_UINT(test_t, a),
        UARTIUM_FIELD_INT(test_t, b),
        UARTIUM_FIELD_FLOAT(test_t, c),
        UARTIUM_FIELD_STRING(test_t, s),
    };

    ASSERT(uartium_buffer_struct_fields(&t, fields, sizeof(fields)/sizeof(fields[0]), UARTIUM_EVENT_INFO, NULL) == UARTIUM_STATUS_OK, "buffer_struct_fields failed");

    const uint8_t *bufptr = NULL;
    size_t buflen = 0;
    ASSERT(uartium_get_buffer(&bufptr, &buflen) == UARTIUM_STATUS_OK, "get_buffer failed");
    const char *actual = (const char*)(bufptr); // library writes starting at buffer[0]
    const char expected[] = "[INFO] a:u=123 b:i=-5 c:f=1.235 s:s=\"hello\"";

    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "[format] Actual:   '%s'\n", actual);
        fprintf(stderr, "[format] Expected: '%s'\n", expected);
        fprintf(stderr, "Buffer len=%zu, raw bytes:\n", buflen);
        for (size_t j = 0; j < (buflen > 64 ? 64 : buflen); ++j) {
            fprintf(stderr, "%02X ", bufptr[j]);
        }
        fprintf(stderr, "\n");
        ASSERT(0, "Formatted output mismatch");
    }

    ASSERT(uartium_flush() == UARTIUM_STATUS_OK, "flush failed");
    ASSERT(bufptr[0] == '\0', "Buffer not cleared after flush");
    ASSERT(uartium_deinit() == UARTIUM_STATUS_OK, "deinit failed");
    return 0;
}

static int test_accumulate_entries(void)
{
    char buffer[512];
    uartium_config_t cfg = { (uint8_t*)buffer, sizeof(buffer), NULL };
    ASSERT(uartium_init(&cfg) == UARTIUM_STATUS_OK, "uartium_init failed");

    typedef struct { uint32_t a; int b; float c; char* s; } test_t;

    test_t t1 = { 123u, -5, 1.2345f, "hello" };
    test_t t2 = { 200u, 3, 2.0f, "world" };

    const uartium_field_t fields[] = {
        UARTIUM_FIELD_UINT(test_t, a),
        UARTIUM_FIELD_INT(test_t, b),
        UARTIUM_FIELD_FLOAT(test_t, c),
        UARTIUM_FIELD_STRING(test_t, s),
    };

    ASSERT(uartium_buffer_struct_fields(&t1, fields, 4, UARTIUM_EVENT_INFO, NULL) == UARTIUM_STATUS_OK, "buffer_struct_fields t1 failed");
    ASSERT(uartium_buffer_struct_fields(&t2, fields, 4, UARTIUM_EVENT_INFO, NULL) == UARTIUM_STATUS_OK, "buffer_struct_fields t2 failed");

    const uint8_t *bufptr = NULL;
    size_t buflen = 0;
    ASSERT(uartium_get_buffer(&bufptr, &buflen) == UARTIUM_STATUS_OK, "get_buffer failed");
    const char *actual = (const char*)(bufptr);
    const char expected[] = "[INFO] a:u=123 b:i=-5 c:f=1.235 s:s=\"hello\"\n[INFO] a:u=200 b:i=3 c:f=2.000 s:s=\"world\"";

    if (strcmp(actual, expected) != 0) {
        fprintf(stderr, "[accum] Actual:   '%s'\n", actual);
        fprintf(stderr, "[accum] Expected: '%s'\n", expected);
        ASSERT(0, "Accumulated entries mismatch");
    }

    ASSERT(uartium_flush() == UARTIUM_STATUS_OK, "flush failed");
    ASSERT(bufptr[0] == '\0', "Buffer not cleared after flush (accum)");
        ASSERT(uartium_deinit() == UARTIUM_STATUS_OK, "deinit failed");
    return 0;
}

static int test_overflow_behavior(void)
{
    /* Use very small buffer and a string that's too large to fit. Expect
       the library to flush and then, if the field is still too large,
       leave the buffer empty and return without crashing. */
    char buffer[16];
    uartium_config_t cfg = { (uint8_t*)buffer, sizeof(buffer), NULL };
    ASSERT(uartium_init(&cfg) == UARTIUM_STATUS_OK, "uartium_init failed");

    typedef struct { char* s; } big_t;
    /* create a long string */
    char *longs = malloc(128);
    memset(longs, 'A', 127);
    longs[127] = '\0';

    big_t b = { longs };
    const uartium_field_t fields[] = { UARTIUM_FIELD_STRING(big_t, s) };

    /* This should not crash; after failure the buffer should be empty */
    uartium_status_t st = uartium_buffer_struct_fields(&b, fields, 1, UARTIUM_EVENT_INFO, NULL);
    ASSERT(st == UARTIUM_STATUS_OK || st == UARTIUM_STATUS_BUFFER_OVERFLOW, "buffer_struct_fields unexpected result");
    const uint8_t *bufptr = NULL;
    size_t buflen = 0;
    ASSERT(uartium_get_buffer(&bufptr, &buflen) == UARTIUM_STATUS_OK, "get_buffer failed");
    ASSERT(bufptr[0] == '\0', "Buffer should be empty after too-large field");

    free(longs);
        ASSERT(uartium_deinit() == UARTIUM_STATUS_OK, "deinit failed");
    return 0;
}

int main(void)
{
    if (test_format_and_flush() != 0) return 1;
    if (test_accumulate_entries() != 0) return 1;
    if (test_overflow_behavior() != 0) return 1;

    printf("All tests passed.\n");
    return 0;
}

