#ifndef SOURCE_OBSERVER_H
#define SOURCE_OBSERVER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    SCENARIO_NORMAL,
    SCENARIO_LEAKAGE_DRIFT,
    SCENARIO_EMISSION_OSCILLATION,
    SCENARIO_KV_RIPPLE,
    SCENARIO_THERMAL_DRIFT,
    SCENARIO_CABLE_INTERMITTENCY,
    SCENARIO_OPTICAL_ONLY
} scenario_t;

typedef enum { PHASE_RAMP, PHASE_SETTLE, PHASE_EXPOSURE } phase_t;

enum {
    QUALITY_ALL_VALID = 0u,
    QUALITY_ELECTRICAL_GAP = 1u << 0,
    QUALITY_IMAGE_GAP = 1u << 1,
    QUALITY_BAD_SPOT_FIT = 1u << 2,
    QUALITY_TIMING_INVALID = 1u << 3,
    QUALITY_NONFINITE_VALUE = 1u << 4
};

enum {
    DIAG_NONE = 0u,
    DIAG_SPOT_UNSTABLE = 1u << 0,
    DIAG_LEAKAGE_ASSOCIATED = 1u << 1,
    DIAG_EMISSION_ASSOCIATED = 1u << 2,
    DIAG_KV_ASSOCIATED = 1u << 3,
    DIAG_THERMAL_ASSOCIATED = 1u << 4,
    DIAG_OPTICAL_OR_MECHANICAL = 1u << 5,
    DIAG_DATA_QUALITY = 1u << 6,
    DIAG_NUMERIC_ERROR = 1u << 7
};

typedef struct {
    uint64_t timestamp_us;
    uint32_t exposure_id;
    phase_t phase;
    double kv_command;
    double kv_measured;
    double tube_current_ma;
    double return_residual_ua;
    double filament_current_a;
    double source_temperature_c;
    double vacuum_mbar;
    double xray_flux_norm;
    double spot_sigma_x_um;
    double spot_sigma_y_um;
    double spot_centroid_x_um;
    double spot_centroid_y_um;
    double spot_fit_r2;
    uint32_t quality;
} source_sample_t;

typedef struct {
    size_t window;
    double spot_cv_limit;
    double leakage_corr_limit;
    double electrical_corr_limit;
    double thermal_corr_limit;
    double minimum_fit_r2;
} observer_config_t;

#define OBSERVER_MAX_WINDOW 128u

typedef struct {
    observer_config_t config;
    source_sample_t samples[OBSERVER_MAX_WINDOW];
    size_t count;
    size_t head;
    uint64_t accepted_count;
    uint64_t rejected_count;
} observer_t;

typedef struct {
    uint32_t flags;
    double spot_cv;
    double corr_spot_leakage;
    double corr_spot_current;
    double corr_spot_kv_error;
    double corr_spot_temperature;
} diagnostic_result_t;

typedef struct {
    scenario_t scenario;
    uint64_t sample_index;
    uint32_t seed;
    double sample_rate_hz;
} simulator_t;

observer_config_t observer_default_config(void);
void observer_init(observer_t *observer, observer_config_t config);
diagnostic_result_t observer_push(observer_t *observer, source_sample_t sample);

void simulator_init(simulator_t *sim, scenario_t scenario, double sample_rate_hz,
                    uint32_t seed);
source_sample_t simulator_next(simulator_t *sim);
const char *scenario_name(scenario_t scenario);
bool scenario_parse(const char *text, scenario_t *scenario);
void diagnostic_flags_text(uint32_t flags, char *buffer, size_t size);

#endif
