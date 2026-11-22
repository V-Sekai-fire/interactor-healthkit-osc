// SPDX-License-Identifier: MIT
// Writes each packet as hex, one per line, for check.py to compare with an independent encoder.
#include "osc.h"
#include <stdio.h>

static void hex(const char *name, const uint8_t *b, size_t n) {
    printf("%s ", name);
    for (size_t i = 0; i < n; i++) printf("%02x", b[i]);
    printf("\n");
}

int main(void) {
    uint8_t b[128];
    size_t n;
    n = osc_heart_rate(b, sizeof b, 72);                       hex("hr", b, n);
    n = osc_heart_rate_connected(b, sizeof b, 1);              hex("conn_t", b, n);
    n = osc_heart_rate_connected(b, sizeof b, 0);              hex("conn_f", b, n);
    n = osc_heart_rate_percent(b, sizeof b, 72.0f / 255.0f);   hex("pct", b, n);
    n = osc_presence(b, sizeof b, 1795382657123ULL, 72.0f, 0.9f, 1800); hex("p", b, n);
    printf("conf %.4f %.4f %.4f %.4f\n", osc_confidence(0), osc_confidence(30000), osc_confidence(60000), osc_confidence(90000));
    n = osc_heart_rate(b, 8, 72);                              printf("small %zu\n", n);
    return 0;
}
