#include "exposure_sync.h"

#include <math.h>
#include <stdio.h>

static int failures;
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); ++failures; \
} } while (0)

static t8_diagnostic_scan_t scan(uint64_t time, double kv) {
    return (t8_diagnostic_scan_t){
        .timestamp_us = time, .kv_measured = kv, .tube_current_ma = 0.2,
        .return_residual_ua = 0.3, .filament_current_a = 1.5,
        .source_temperature_c = 27.0, .vacuum_mbar = 1e-7,
        .xray_flux_norm = 1.0
    };
}

static detector_observation_t detector(uint64_t begin, uint64_t end) {
    return (detector_observation_t){
        .exposure_id = 42u, .request_timestamp_us = begin,
        .completion_timestamp_us = end, .phase = PHASE_EXPOSURE,
        .kv_command = 50.0, .spot_sigma_x_um = 4.0,
        .spot_sigma_y_um = 5.0, .spot_fit_r2 = 0.99
    };
}

int main(void) {
    exposure_sync_t sync;
    exposure_sync_init(&sync, 5u, 15000u);
    for (size_t i = 0u; i < 10u; ++i)
        CHECK(exposure_sync_push_t8(&sync, scan(1000000u + i * 10000u, 49.0 + (double)i)) == 0);
    exposure_alignment_t aligned;
    detector_observation_t d = detector(1000000u, 1090000u);
    CHECK(exposure_sync_align(&sync, &d, &aligned) == 0);
    CHECK(aligned.t8_sample_count == 10u);
    CHECK(fabs(aligned.sample.kv_measured - 53.5) < 1e-12);
    CHECK(aligned.sample.exposure_id == 42u);
    CHECK(aligned.maximum_observed_gap_us == 10000u);

    d = detector(1000000u, 1020000u);
    CHECK(exposure_sync_align(&sync, &d, &aligned) == 1);
    CHECK((aligned.quality & QUALITY_ELECTRICAL_GAP) != 0u);
    d = detector(1090000u, 1000000u);
    CHECK(exposure_sync_align(&sync, &d, &aligned) == 1);
    CHECK((aligned.quality & QUALITY_TIMING_INVALID) != 0u);
    d = detector(1000000u, 1090000u);
    d.spot_fit_r2 = NAN;
    CHECK(exposure_sync_align(&sync, &d, &aligned) == 1);
    CHECK((aligned.quality & QUALITY_NONFINITE_VALUE) != 0u);
    if (failures == 0) puts("PASS: exposure synchronization contract");
    return failures == 0 ? 0 : 1;
}
