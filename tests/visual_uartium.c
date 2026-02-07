#include <stdio.h>
#include <string.h>
#include "uartium.h"

// Simple visual test: initialize, write a couple of entries and print buffer
int main(void)
{
    char buffer[256];
    uartium_config_t cfg = { (uint8_t*)buffer, sizeof(buffer), NULL };
    uartium_status_t st = uartium_init(&cfg);
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "uartium_init failed: %d\n", (int)st);
        return 1;
    }

    typedef struct { uint32_t a; int b; float c; char* s; } sample_t;
    sample_t s1 = { 42u, -7, 3.1415f, "hello" };
    sample_t s2 = { 100u, 0, 2.5f, "world" };

    const uartium_field_t fields[] = {
        UARTIUM_FIELD_UINT(sample_t, a),
        UARTIUM_FIELD_INT(sample_t, b),
        UARTIUM_FIELD_FLOAT(sample_t, c),
        UARTIUM_FIELD_STRING(sample_t, s),
    };

    st = uartium_buffer_struct_fields(&s1, fields, 4);
    if (st != UARTIUM_STATUS_OK && st != UARTIUM_STATUS_BUFFER_OVERFLOW) {
        fprintf(stderr, "buffer_fields failed: %d\n", (int)st);
        return 1;
    }

    st = uartium_buffer_struct_fields(&s2, fields, 4);
    if (st != UARTIUM_STATUS_OK && st != UARTIUM_STATUS_BUFFER_OVERFLOW) {
        fprintf(stderr, "buffer_fields failed: %d\n", (int)st);
        return 1;
    }

    const uint8_t *bufptr = NULL;
    size_t buflen = 0;
    st = uartium_get_buffer(&bufptr, &buflen);
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "get_buffer failed: %d\n", (int)st);
        return 1;
    }

    printf("--- UARTIUM BUFFER (len=%zu) ---\n", buflen);
    if (buflen == 0 || bufptr[0] == '\0') {
        printf("<empty>\n");
    } else {
        // print as string and show hex dump for clarity
        printf("%s\n", (const char*)bufptr);
        printf("--- raw bytes ---\n");
        for (size_t i = 0; i < buflen; ++i) printf("%02X ", bufptr[i]);
        printf("\n");
    }

    st = uartium_flush();
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "flush failed: %d\n", (int)st);
        return 1;
    }

    printf("--- AFTER FLUSH ---\n");
    st = uartium_get_buffer(&bufptr, &buflen);
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "get_buffer failed after flush: %d\n", (int)st);
        return 1;
    }
    printf("len=%zu, first byte=0x%02X\n", buflen, (unsigned)bufptr[0]);

    st = uartium_deinit();
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "deinit failed: %d\n", (int)st);
        return 1;
    }

    return 0;
}
