#include "source_observer.h"

#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

observer_config_t observer_default_config(void) {
    return (observer_config_t){
        .window = 64u,
        .spot_cv_limit = 0.015,
        .leakage_corr_limit = 0.70,
        .electrical_corr_limit = 0.65,
        .thermal_corr_limit = 0.70,
        .minimum_fit_r2 = 0.92
    };
}

void observer_init(observer_t *observer, observer_config_t config) {
    if (observer == NULL) return;
    memset(observer, 0, sizeof(*observer));
    const observer_config_t defaults = observer_default_config();
    if (config.window < 8u) config.window = 8u;
    if (config.window > OBSERVER_MAX_WINDOW) config.window = OBSERVER_MAX_WINDOW;
    if (!isfinite(config.spot_cv_limit) || config.spot_cv_limit < 0.0)
        config.spot_cv_limit = defaults.spot_cv_limit;
    if (!isfinite(config.leakage_corr_limit) || config.leakage_corr_limit < 0.0 ||
        config.leakage_corr_limit > 1.0)
        config.leakage_corr_limit = defaults.leakage_corr_limit;
    if (!isfinite(config.electrical_corr_limit) || config.electrical_corr_limit < 0.0 ||
        config.electrical_corr_limit > 1.0)
        config.electrical_corr_limit = defaults.electrical_corr_limit;
    if (!isfinite(config.thermal_corr_limit) || config.thermal_corr_limit < 0.0 ||
        config.thermal_corr_limit > 1.0)
        config.thermal_corr_limit = defaults.thermal_corr_limit;
    if (!isfinite(config.minimum_fit_r2) || config.minimum_fit_r2 < 0.0 ||
        config.minimum_fit_r2 > 1.0)
        config.minimum_fit_r2 = defaults.minimum_fit_r2;
    observer->config = config;
}

static double spot_metric(const source_sample_t *s) {
    return hypot(s->spot_sigma_x_um, s->spot_sigma_y_um) / sqrt(2.0);
}

static double correlation(const double *x, const double *y, size_t n) {
    if (x == NULL || y == NULL || n < 2u) return 0.0;
    double mean_x = 0.0, mean_y = 0.0;
    double sum_xx = 0.0, sum_yy = 0.0, sum_xy = 0.0;
    for (size_t i = 0; i < n; ++i) {
        const double count = (double)(i + 1u);
        const double dx = x[i] - mean_x;
        const double dy = y[i] - mean_y;
        mean_x += dx / count;
        mean_y += dy / count;
        sum_xx += dx * (x[i] - mean_x);
        sum_yy += dy * (y[i] - mean_y);
        sum_xy += dx * (y[i] - mean_y);
    }
    if (sum_xx <= DBL_EPSILON || sum_yy <= DBL_EPSILON) return 0.0;
    const double value = sum_xy / sqrt(sum_xx * sum_yy);
    if (!isfinite(value)) return 0.0;
    return fmax(-1.0, fmin(1.0, value));
}

diagnostic_result_t observer_push(observer_t *o, source_sample_t sample) {
    diagnostic_result_t r = {0};
    if (o == NULL) {
        r.flags = DIAG_DATA_QUALITY | DIAG_NUMERIC_ERROR;
        return r;
    }
    const int finite_values =
        isfinite(sample.kv_command) && isfinite(sample.kv_measured) &&
        isfinite(sample.tube_current_ma) && isfinite(sample.return_residual_ua) &&
        isfinite(sample.source_temperature_c) &&
        isfinite(sample.spot_sigma_x_um) && isfinite(sample.spot_sigma_y_um) &&
        isfinite(sample.spot_fit_r2);
    const int valid_fit = sample.spot_fit_r2 >= o->config.minimum_fit_r2 &&
                          sample.spot_sigma_x_um > 0.0 &&
                          sample.spot_sigma_y_um > 0.0;
    if (sample.quality != QUALITY_ALL_VALID || !finite_values || !valid_fit) {
        ++o->rejected_count;
        r.flags = DIAG_DATA_QUALITY;
        if (!finite_values) r.flags |= DIAG_NUMERIC_ERROR;
        return r;
    }

    o->samples[o->head] = sample;
    o->head = (o->head + 1u) % o->config.window;
    if (o->count < o->config.window) ++o->count;
    ++o->accepted_count;
    if (o->count < o->config.window) return r;

    double spot[OBSERVER_MAX_WINDOW], leak[OBSERVER_MAX_WINDOW];
    double current[OBSERVER_MAX_WINDOW], kv_error[OBSERVER_MAX_WINDOW];
    double temperature[OBSERVER_MAX_WINDOW];
    double mean = 0.0, m2 = 0.0;
    for (size_t i = 0; i < o->count; ++i) {
        const source_sample_t *s = &o->samples[(o->head + i) % o->config.window];
        spot[i] = spot_metric(s); leak[i] = s->return_residual_ua;
        current[i] = s->tube_current_ma;
        kv_error[i] = s->kv_measured - s->kv_command;
        temperature[i] = s->source_temperature_c;
        const double delta = spot[i] - mean;
        mean += delta / (double)(i + 1u);
        m2 += delta * (spot[i] - mean);
    }
    if (!isfinite(mean) || fabs(mean) <= DBL_EPSILON || !isfinite(m2) || m2 < 0.0) {
        r.flags |= DIAG_DATA_QUALITY | DIAG_NUMERIC_ERROR;
        return r;
    }
    r.spot_cv = sqrt(m2 / (double)(o->count - 1u)) / fabs(mean);
    r.corr_spot_leakage = correlation(spot, leak, o->count);
    r.corr_spot_current = correlation(spot, current, o->count);
    r.corr_spot_kv_error = correlation(spot, kv_error, o->count);
    r.corr_spot_temperature = correlation(spot, temperature, o->count);
    if (!isfinite(r.spot_cv) || !isfinite(r.corr_spot_leakage) ||
        !isfinite(r.corr_spot_current) || !isfinite(r.corr_spot_kv_error) ||
        !isfinite(r.corr_spot_temperature)) {
        r.flags |= DIAG_DATA_QUALITY | DIAG_NUMERIC_ERROR;
        return r;
    }
    if (r.spot_cv <= o->config.spot_cv_limit) return r;

    r.flags |= DIAG_SPOT_UNSTABLE;
    if (fabs(r.corr_spot_leakage) >= o->config.leakage_corr_limit)
        r.flags |= DIAG_LEAKAGE_ASSOCIATED;
    if (fabs(r.corr_spot_current) >= o->config.electrical_corr_limit)
        r.flags |= DIAG_EMISSION_ASSOCIATED;
    if (fabs(r.corr_spot_kv_error) >= o->config.electrical_corr_limit)
        r.flags |= DIAG_KV_ASSOCIATED;
    if (fabs(r.corr_spot_temperature) >= o->config.thermal_corr_limit)
        r.flags |= DIAG_THERMAL_ASSOCIATED;
    if ((r.flags & (DIAG_LEAKAGE_ASSOCIATED | DIAG_EMISSION_ASSOCIATED |
                    DIAG_KV_ASSOCIATED | DIAG_THERMAL_ASSOCIATED)) == 0u)
        r.flags |= DIAG_OPTICAL_OR_MECHANICAL;
    return r;
}

void diagnostic_flags_text(uint32_t flags, char *buffer, size_t size) {
    const struct { uint32_t bit; const char *name; } items[] = {
        {DIAG_SPOT_UNSTABLE, "SPOT_UNSTABLE"}, {DIAG_LEAKAGE_ASSOCIATED, "LEAKAGE_ASSOC"},
        {DIAG_EMISSION_ASSOCIATED, "EMISSION_ASSOC"}, {DIAG_KV_ASSOCIATED, "KV_ASSOC"},
        {DIAG_THERMAL_ASSOCIATED, "THERMAL_ASSOC"},
        {DIAG_OPTICAL_OR_MECHANICAL, "OPTICAL_MECH"}, {DIAG_DATA_QUALITY, "DATA_QUALITY"},
        {DIAG_NUMERIC_ERROR, "NUMERIC_ERROR"}
    };
    if (size == 0u) return;
    buffer[0] = '\0';
    if (flags == 0u) { (void)snprintf(buffer, size, "NONE"); return; }
    for (size_t i = 0; i < sizeof(items) / sizeof(items[0]); ++i) {
        if ((flags & items[i].bit) == 0u) continue;
        const size_t used = strlen(buffer);
        (void)snprintf(buffer + used, size - used, "%s%s", used ? "|" : "", items[i].name);
    }
}
