#include "exposure_sync.h"

#include <math.h>
#include <string.h>

void exposure_sync_init(exposure_sync_t *sync, size_t minimum_scans,
                        uint64_t maximum_gap_us) {
    if (sync == NULL) return;
    memset(sync, 0, sizeof(*sync));
    sync->minimum_scans = minimum_scans == 0u ? 1u : minimum_scans;
    sync->maximum_gap_us = maximum_gap_us;
}

static int scan_finite(const t8_diagnostic_scan_t *s) {
    return isfinite(s->kv_measured) && isfinite(s->tube_current_ma) &&
           isfinite(s->return_residual_ua) && isfinite(s->filament_current_a) &&
           isfinite(s->source_temperature_c) && isfinite(s->vacuum_mbar) &&
           isfinite(s->xray_flux_norm);
}

int exposure_sync_push_t8(exposure_sync_t *sync, t8_diagnostic_scan_t scan) {
    if (sync == NULL || scan.timestamp_us == 0u) return -1;
    if (!scan_finite(&scan)) scan.quality |= QUALITY_NONFINITE_VALUE;
    sync->scans[sync->head] = scan;
    sync->head = (sync->head + 1u) % EXPOSURE_SYNC_MAX_SCANS;
    if (sync->count < EXPOSURE_SYNC_MAX_SCANS) ++sync->count;
    return 0;
}

static int detector_finite(const detector_observation_t *d) {
    return isfinite(d->kv_command) && isfinite(d->spot_sigma_x_um) &&
           isfinite(d->spot_sigma_y_um) && isfinite(d->spot_centroid_x_um) &&
           isfinite(d->spot_centroid_y_um) && isfinite(d->spot_fit_r2);
}

int exposure_sync_align(const exposure_sync_t *sync,
                        const detector_observation_t *detector,
                        exposure_alignment_t *alignment) {
    if (sync == NULL || detector == NULL || alignment == NULL) return -1;
    memset(alignment, 0, sizeof(*alignment));
    source_sample_t *out = &alignment->sample;
    out->exposure_id = detector->exposure_id;
    out->timestamp_us = detector->completion_timestamp_us;
    out->phase = detector->phase;
    out->kv_command = detector->kv_command;
    out->spot_sigma_x_um = detector->spot_sigma_x_um;
    out->spot_sigma_y_um = detector->spot_sigma_y_um;
    out->spot_centroid_x_um = detector->spot_centroid_x_um;
    out->spot_centroid_y_um = detector->spot_centroid_y_um;
    out->spot_fit_r2 = detector->spot_fit_r2;
    alignment->quality = detector->quality;

    if (detector->request_timestamp_us == 0u ||
        detector->completion_timestamp_us < detector->request_timestamp_us) {
        alignment->quality |= QUALITY_TIMING_INVALID;
    }
    if (!detector_finite(detector)) alignment->quality |= QUALITY_NONFINITE_VALUE;

    uint64_t previous = 0u;
    for (size_t i = 0u; i < sync->count; ++i) {
        const size_t index = (sync->head + EXPOSURE_SYNC_MAX_SCANS - sync->count + i) %
                             EXPOSURE_SYNC_MAX_SCANS;
        const t8_diagnostic_scan_t *s = &sync->scans[index];
        if (s->timestamp_us < detector->request_timestamp_us ||
            s->timestamp_us > detector->completion_timestamp_us)
            continue;
        if (previous != 0u) {
            const uint64_t gap = s->timestamp_us - previous;
            if (gap > alignment->maximum_observed_gap_us)
                alignment->maximum_observed_gap_us = gap;
        }
        previous = s->timestamp_us;
        ++alignment->t8_sample_count;
        out->kv_measured += s->kv_measured;
        out->tube_current_ma += s->tube_current_ma;
        out->return_residual_ua += s->return_residual_ua;
        out->filament_current_a += s->filament_current_a;
        out->source_temperature_c += s->source_temperature_c;
        out->vacuum_mbar += s->vacuum_mbar;
        out->xray_flux_norm += s->xray_flux_norm;
        alignment->quality |= s->quality;
    }
    if (alignment->t8_sample_count < sync->minimum_scans)
        alignment->quality |= QUALITY_ELECTRICAL_GAP;
    if (sync->maximum_gap_us != 0u &&
        alignment->maximum_observed_gap_us > sync->maximum_gap_us)
        alignment->quality |= QUALITY_ELECTRICAL_GAP;
    if (alignment->t8_sample_count != 0u) {
        const double n = (double)alignment->t8_sample_count;
        out->kv_measured /= n;
        out->tube_current_ma /= n;
        out->return_residual_ua /= n;
        out->filament_current_a /= n;
        out->source_temperature_c /= n;
        out->vacuum_mbar /= n;
        out->xray_flux_norm /= n;
    }
    out->quality = alignment->quality;
    return alignment->quality == QUALITY_ALL_VALID ? 0 : 1;
}
