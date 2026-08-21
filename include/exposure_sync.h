#ifndef EXPOSURE_SYNC_H
#define EXPOSURE_SYNC_H

#include "source_observer.h"

#include <stddef.h>
#include <stdint.h>

#define EXPOSURE_SYNC_MAX_SCANS 4096u

typedef struct {
    uint64_t timestamp_us;
    double kv_measured;
    double tube_current_ma;
    double return_residual_ua;
    double filament_current_a;
    double source_temperature_c;
    double vacuum_mbar;
    double xray_flux_norm;
    uint32_t quality;
} t8_diagnostic_scan_t;

typedef struct {
    uint32_t exposure_id;
    uint64_t request_timestamp_us;
    uint64_t completion_timestamp_us;
    phase_t phase;
    double kv_command;
    double spot_sigma_x_um;
    double spot_sigma_y_um;
    double spot_centroid_x_um;
    double spot_centroid_y_um;
    double spot_fit_r2;
    uint32_t quality;
} detector_observation_t;

typedef struct {
    t8_diagnostic_scan_t scans[EXPOSURE_SYNC_MAX_SCANS];
    size_t count;
    size_t head;
    size_t minimum_scans;
    uint64_t maximum_gap_us;
} exposure_sync_t;

typedef struct {
    source_sample_t sample;
    size_t t8_sample_count;
    uint64_t maximum_observed_gap_us;
    uint32_t quality;
} exposure_alignment_t;

void exposure_sync_init(exposure_sync_t *sync, size_t minimum_scans,
                        uint64_t maximum_gap_us);
int exposure_sync_push_t8(exposure_sync_t *sync, t8_diagnostic_scan_t scan);
int exposure_sync_align(const exposure_sync_t *sync,
                        const detector_observation_t *detector,
                        exposure_alignment_t *alignment);

#endif
