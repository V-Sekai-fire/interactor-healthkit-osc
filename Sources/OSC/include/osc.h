// SPDX-License-Identifier: MIT
#ifndef HEARTRATE_OSC_H
#define HEARTRATE_OSC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Each encoder writes one OSC packet into buf and returns its length in bytes,
// or 0 when cap is too small. The packets follow spec/asyncapi.yaml.

// /avatar/parameters/HR  ,i
size_t osc_heart_rate(uint8_t *buf, size_t cap, int32_t bpm);

// /avatar/parameters/isHRConnected  ,T or ,F
size_t osc_heart_rate_connected(uint8_t *buf, size_t cap, int connected);

// /avatar/parameters/HRPercent  ,f  (bpm divided by 255)
size_t osc_heart_rate_percent(uint8_t *buf, size_t cap, float fraction);

// /presence/P  ,ffi  inside a bundle whose time tag is the sample time
// (milliseconds since the Unix epoch).
size_t osc_presence(uint8_t *buf, size_t cap, uint64_t sample_unix_ms,
                    float bpm, float confidence, int32_t age_ms);

// Send one packet over UDP to host:port. Returns 0 on success, -1 on failure.
int osc_udp_send(const char *host, uint16_t port, const uint8_t *buf, size_t len);

#ifdef __cplusplus
}
#endif

#endif
