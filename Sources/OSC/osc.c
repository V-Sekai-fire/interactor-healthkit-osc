// SPDX-License-Identifier: MIT
#include "osc.h"

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

typedef struct {
    uint8_t *buf;
    size_t cap;
    size_t len;
    int ok;
} writer_t;

static void put(writer_t *w, const void *p, size_t n) {
    if (!w->ok || w->len + n > w->cap) { w->ok = 0; return; }
    memcpy(w->buf + w->len, p, n);
    w->len += n;
}

static void put_u32(writer_t *w, uint32_t v) {
    uint8_t b[4] = { (uint8_t)(v >> 24), (uint8_t)(v >> 16), (uint8_t)(v >> 8), (uint8_t)v };
    put(w, b, 4);
}

// An OSC string is its bytes, a NUL, and NULs up to a multiple of four.
static void put_str(writer_t *w, const char *s) {
    size_t n = strlen(s) + 1;
    static const uint8_t zero[4] = {0, 0, 0, 0};
    put(w, s, n - 1);
    put(w, zero, 4 - ((n - 1) % 4));
}

static void put_f32(writer_t *w, float f) {
    uint32_t v;
    memcpy(&v, &f, 4);
    put_u32(w, v);
}

static size_t finish(const writer_t *w) { return w->ok ? w->len : 0; }

size_t osc_heart_rate(uint8_t *buf, size_t cap, int32_t bpm) {
    writer_t w = { buf, cap, 0, 1 };
    put_str(&w, "/avatar/parameters/HR");
    put_str(&w, ",i");
    put_u32(&w, (uint32_t)bpm);
    return finish(&w);
}

size_t osc_heart_rate_connected(uint8_t *buf, size_t cap, int connected) {
    writer_t w = { buf, cap, 0, 1 };
    put_str(&w, "/avatar/parameters/isHRConnected");
    put_str(&w, connected ? ",T" : ",F");
    return finish(&w);
}

size_t osc_heart_rate_percent(uint8_t *buf, size_t cap, float fraction) {
    writer_t w = { buf, cap, 0, 1 };
    put_str(&w, "/avatar/parameters/HRPercent");
    put_str(&w, ",f");
    put_f32(&w, fraction);
    return finish(&w);
}

size_t osc_presence(uint8_t *buf, size_t cap, uint64_t sample_unix_ms,
                    float bpm, float confidence, int32_t age_ms) {
    uint8_t msg[64];
    writer_t m = { msg, sizeof msg, 0, 1 };
    put_str(&m, "/presence/P");
    put_str(&m, ",ffi");
    put_f32(&m, bpm);
    put_f32(&m, confidence);
    put_u32(&m, (uint32_t)age_ms);
    if (!m.ok) return 0;

    // NTP time tag: seconds since 1900 in the high word, binary fraction in the low word.
    uint64_t secs = sample_unix_ms / 1000 + 2208988800ULL;
    uint64_t frac = ((sample_unix_ms % 1000) << 32) / 1000;

    writer_t w = { buf, cap, 0, 1 };
    put_str(&w, "#bundle");
    put_u32(&w, (uint32_t)secs);
    put_u32(&w, (uint32_t)frac);
    put_u32(&w, (uint32_t)m.len);
    put(&w, msg, m.len);
    return finish(&w);
}

int osc_udp_send(const char *host, uint16_t port, const uint8_t *buf, size_t len) {
    char service[8];
    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_family = AF_UNSPEC;
    snprintf(service, sizeof service, "%u", (unsigned)port);
    if (getaddrinfo(host, service, &hints, &res) != 0 || !res) return -1;
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    int rc = -1;
    if (fd >= 0) {
        rc = sendto(fd, buf, len, 0, res->ai_addr, res->ai_addrlen) == (ssize_t)len ? 0 : -1;
        close(fd);
    }
    freeaddrinfo(res);
    return rc;
}
