#include <stdio.h>
#include <string.h>
#include "uartium.h"

int main(void)
{
    char buffer[128];
    uartium_config_t cfg = { (uint8_t*)buffer, sizeof(buffer), NULL };
    uartium_status_t st = uartium_init(&cfg);
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "uartium_init failed: %d\n", (int)st);
        return 1;
    }

    st = uartium_buffer_message(UARTIUM_EVENT_INFO, "Something");
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "uartium_buffer_message failed: %d\n", (int)st);
        return 1;
    }

    const uint8_t *bufptr = NULL;
    size_t buflen = 0;
    st = uartium_get_buffer(&bufptr, &buflen);
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "uartium_get_buffer failed: %d\n", (int)st);
        return 1;
    }

    printf("After buffer_message:\n");
    if (buflen == 0 || bufptr[0] == '\0') {
        printf("<empty>\n");
    } else {
        printf("%s", (const char*)bufptr);
        printf("\n--- raw bytes ---\n");
        for (size_t i = 0; i < buflen; ++i) printf("%02X ", bufptr[i]);
        printf("\n");
    }

    /* log_message should flush after writing */
    st = uartium_log_message(UARTIUM_EVENT_INFO, "Other");
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "uartium_log_message failed: %d\n", (int)st);
        return 1;
    }

    printf("After log_message (should be flushed):\n");
    st = uartium_get_buffer(&bufptr, &buflen);
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "uartium_get_buffer failed: %d\n", (int)st);
        return 1;
    }
    printf("len=%zu, first=0x%02X\n", buflen, (unsigned)(bufptr[0]));

    st = uartium_deinit();
    if (st != UARTIUM_STATUS_OK) {
        fprintf(stderr, "uartium_deinit failed: %d\n", (int)st);
        return 1;
    }

    return 0;
}
