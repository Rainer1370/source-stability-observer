#include "source_observer.h"

#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846

static double noise(simulator_t *s) {
    s->seed = 1664525u * s->seed + 1013904223u;
    return ((double)(s->seed >> 8) / 16777215.0) * 2.0 - 1.0;
}

void simulator_init(simulator_t *sim, scenario_t scenario, double rate, uint32_t seed) {
    *sim = (simulator_t){.scenario = scenario, .seed = seed, .sample_rate_hz = rate};
}

source_sample_t simulator_next(simulator_t *s) {
    const double t = (double)s->sample_index / s->sample_rate_hz;
    source_sample_t x = {
        .timestamp_us = (uint64_t)llround(t * 1000000.0),
        .exposure_id = (uint32_t)(s->sample_index / (uint64_t)(5.0 * s->sample_rate_hz)),
        .phase = t < 1.0 ? PHASE_RAMP : (t < 2.0 ? PHASE_SETTLE : PHASE_EXPOSURE),
        .kv_command = 50.0, .kv_measured = 50.0 + 0.01 * noise(s),
        .tube_current_ma = 0.200 + 0.0005 * noise(s),
        .return_residual_ua = 0.5 + 0.05 * noise(s),
        .filament_current_a = 1.80 + 0.002 * noise(s),
        .source_temperature_c = 25.0 + 0.002 * t + 0.01 * noise(s),
        .vacuum_mbar = 1.0e-7 * (1.0 + 0.01 * noise(s)),
        .xray_flux_norm = 1.0 + 0.002 * noise(s),
        .spot_sigma_x_um = 5.0 + 0.015 * noise(s),
        .spot_sigma_y_um = 5.1 + 0.015 * noise(s),
        .spot_centroid_x_um = 0.02 * noise(s), .spot_centroid_y_um = 0.02 * noise(s),
        .spot_fit_r2 = 0.995, .quality = QUALITY_ALL_VALID
    };
    const double slow = sin(2.0 * PI * 0.20 * t);
    const double fast = sin(2.0 * PI * 2.0 * t);
    switch (s->scenario) {
        case SCENARIO_LEAKAGE_DRIFT:
            x.return_residual_ua += 2.0 * slow;
            x.spot_sigma_x_um += 0.45 * slow; x.spot_sigma_y_um += 0.35 * slow;
            break;
        case SCENARIO_EMISSION_OSCILLATION:
            x.tube_current_ma += 0.018 * fast;
            x.xray_flux_norm += 0.08 * fast; x.spot_sigma_x_um += 0.55 * fast;
            break;
        case SCENARIO_KV_RIPPLE:
            x.kv_measured += 0.75 * fast;
            x.spot_sigma_x_um += 0.45 * fast; x.spot_sigma_y_um += 0.30 * fast;
            break;
        case SCENARIO_THERMAL_DRIFT:
            x.source_temperature_c += 0.08 * t + 3.0 * slow;
            x.spot_sigma_x_um += 0.025 * t + 0.70 * slow;
            x.spot_sigma_y_um += 0.020 * t + 0.55 * slow;
            break;
        case SCENARIO_CABLE_INTERMITTENCY:
            if (((unsigned)t % 4u) == 3u) {
                x.return_residual_ua += 4.0; x.tube_current_ma -= 0.025;
                x.spot_sigma_x_um += 0.8; x.xray_flux_norm -= 0.12;
            }
            break;
        case SCENARIO_OPTICAL_ONLY:
            x.spot_sigma_x_um += 0.55 * slow; x.spot_sigma_y_um += 0.30 * slow;
            x.spot_centroid_x_um += 0.8 * slow;
            break;
        case SCENARIO_NORMAL: break;
    }
    ++s->sample_index;
    return x;
}

static const char *const names[] = {"normal", "leakage", "emission", "kv-ripple",
                                    "thermal", "cable", "optical"};
const char *scenario_name(scenario_t s) { return names[(size_t)s]; }
bool scenario_parse(const char *text, scenario_t *scenario) {
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i)
        if (strcmp(text, names[i]) == 0) { *scenario = (scenario_t)i; return true; }
    return false;
}
