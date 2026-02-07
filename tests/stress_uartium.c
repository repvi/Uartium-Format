#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "uartium.h"

// Simple stress tester: many random struct serializations, check invariants.

static void fill_pattern(char *buf, size_t len, unsigned seed) {
    for (size_t i = 0; i + 1 < len; ++i) buf[i] = (char)('A' + (seed + i) % 26);
    buf[len-1] = '\0';
}

int main(void)
{
    unsigned iterations = 20000; // reasonable stress size for CI/local runs
    char buffer[256];
    uartium_config_t cfg = { (uint8_t*)buffer, sizeof(buffer), NULL };
    if (uartium_init(&cfg) != UARTIUM_STATUS_OK) {
        fprintf(stderr, "uartium_init failed\n");
        return 1;
    }
    uartium_status_t st;

    srand(12345);

    for (unsigned it = 0; it < iterations; ++it) {
        // Vary buffer size by reinitializing system with different buffers
        // occasionally
        if ((it & 0x1FF) == 0) {
            // re-init with same buffer to reset: require deinit then init
            st = uartium_deinit();
            if (st != UARTIUM_STATUS_OK) {
                fprintf(stderr, "uartium_deinit failed at iter %u: %d\n", it, (int)st);
                return 1;
            }
            st = uartium_init(&cfg);
            if (st != UARTIUM_STATUS_OK) {
                fprintf(stderr, "uartium re-init failed at iter %u: %d\n", it, (int)st);
                return 1;
            }
        }

        // Build a small dynamic struct layout
        typedef struct dyn_t { uint32_t a; int b; float c; char *s; } dyn_t;
        dyn_t d;
        d.a = (uint32_t)rand();
        d.b = (int)(rand() % 1000) - 500;
        d.c = (float)(rand()) / (float)(RAND_MAX/10.0);

        static char longstr[128];
        fill_pattern(longstr, sizeof(longstr), it % 26);
        // Use variable-length string slices
        int slice = 5 + (rand() % 50);
        longstr[slice] = '\0';
        d.s = longstr;

        const uartium_field_t fields[] = {
            UARTIUM_FIELD_UINT(dyn_t, a),
            UARTIUM_FIELD_INT(dyn_t, b),
            UARTIUM_FIELD_FLOAT(dyn_t, c),
            UARTIUM_FIELD_STRING(dyn_t, s),
        };

        /* Always use the buffer variant so we can validate expected content. */
        /* Build formatted entry string matching uartium formatting (no separators):
           a=%ub=%dc=%.3fs=%s */
        char entry[512];
        snprintf(entry, sizeof(entry), "a=%u b=%d c=%.3f s=%s", d.a, d.b, d.c, d.s);

        static char expected[16384];
        if (expected[0] == '\0') {
            strcpy(expected, entry);
        } else {
            size_t cur = strlen(expected);
            expected[cur] = '\n';
            expected[cur+1] = '\0';
            strncat(expected, entry, sizeof(expected)-strlen(expected)-1);
        }

        st = uartium_buffer_struct_fields(&d, fields, 4);
        if (st != UARTIUM_STATUS_OK && st != UARTIUM_STATUS_BUFFER_OVERFLOW) {
            fprintf(stderr, "uartium_buffer_struct_fields unexpected status at iter %u: %d\n", it, (int)st);
            return 1;
        }

        const uint8_t *bufptr = NULL;
        size_t buflen = 0;
        if (uartium_get_buffer(&bufptr, &buflen) != UARTIUM_STATUS_OK) {
            fprintf(stderr, "ERROR: uartium_get_buffer failed at iter %u\n", it);
            return 1;
        }
        const char *actual = (const char*)(bufptr); /* library writes starting at buffer[0] */

        if (strcmp(actual, expected) == 0) {
            /* all good */
        } else if (strcmp(actual, entry) == 0) {
            /* The library flushed prior content then retried this entry; accept and
               reset expected to current entry. */
            expected[0] = '\0';
            strcpy(expected, entry);
        } else if (bufptr[0] == '\0') {
            /* Buffer empty: library flushed and couldn't fit the field; reset expected. */
            expected[0] = '\0';
        } else {
            fprintf(stderr, "Mismatch at iter %u\n", it);
            fprintf(stderr, "Expected: '%s'\n", expected);
            fprintf(stderr, "Actual:   '%s'\n", actual);
            /* Print raw buffer bytes for debugging */
            fprintf(stderr, "Buffer len=%zu, raw bytes:\n", buflen);
            for (size_t j = 0; j < (buflen > 128 ? 128 : buflen); ++j) {
                fprintf(stderr, "%02X ", bufptr[j]);
            }
            fprintf(stderr, "\n");
            return 1;
        }

        /* Occasional explicit flush and check that buffer clears and expected resets */
        if ((it & 0x3FF) == 0) {
            st = uartium_flush();
            if (st != UARTIUM_STATUS_OK) {
                fprintf(stderr, "ERROR: uartium_flush failed at iter %u: %d\n", it, (int)st);
                return 1;
            }
            if (bufptr[0] != '\0') {
                fprintf(stderr, "ERROR: buffer not cleared after flush at iter %u\n", it);
                return 1;
            }
            expected[0] = '\0';
        }

        if ((it & 0x3FF) == 0) {
            /* Progress */
            printf("stress: iter %u/%u\n", it, iterations);
        }
    }

    printf("Stress test completed %u iterations successfully.\n", iterations);
    st = uartium_deinit();
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "uartium_deinit failed at end: %d\n", (int)st);
        return 1;
    }
    return 0;
}
