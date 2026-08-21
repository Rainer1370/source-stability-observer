#include "source_observer.h"

#include <math.h>
#include <stdio.h>

static diagnostic_result_t run(scenario_t scenario) {
    simulator_t sim; observer_t observer;
    simulator_init(&sim, scenario, 100.0, 1234u);
    observer_init(&observer, observer_default_config());
    diagnostic_result_t result = {0};
    for (size_t i = 0; i < 1200u; ++i) {
        const diagnostic_result_t instant = observer_push(&observer, simulator_next(&sim));
        result.flags |= instant.flags;
    }
    return result;
}

static int require(const char *name, int condition) {
    if (!condition) { fprintf(stderr, "FAIL: %s\n", name); return 1; }
    printf("PASS: %s\n", name); return 0;
}

static source_sample_t valid_sample(void) {
    simulator_t sim;
    simulator_init(&sim, SCENARIO_NORMAL, 100.0, 4321u);
    return simulator_next(&sim);
}

static int test_quality_gate(void) {
    observer_t observer;
    observer_init(&observer, observer_default_config());
    source_sample_t sample = valid_sample();
    sample.spot_fit_r2 = 0.1;
    sample.quality = QUALITY_BAD_SPOT_FIT;
    const diagnostic_result_t result = observer_push(&observer, sample);
    return (result.flags & DIAG_DATA_QUALITY) != 0u && observer.count == 0u &&
           observer.accepted_count == 0u && observer.rejected_count == 1u;
}

static int test_nonfinite_gate(void) {
    observer_t observer;
    observer_init(&observer, observer_default_config());
    source_sample_t sample = valid_sample();
    sample.kv_measured = NAN;
    const diagnostic_result_t result = observer_push(&observer, sample);
    return (result.flags & DIAG_DATA_QUALITY) != 0u &&
           (result.flags & DIAG_NUMERIC_ERROR) != 0u && observer.count == 0u;
}

static int test_config_sanitization(void) {
    observer_t observer;
    observer_config_t config = observer_default_config();
    config.window = OBSERVER_MAX_WINDOW + 100u;
    config.spot_cv_limit = -1.0;
    config.minimum_fit_r2 = 2.0;
    observer_init(&observer, config);
    return observer.config.window == OBSERVER_MAX_WINDOW &&
           observer.config.spot_cv_limit == observer_default_config().spot_cv_limit &&
           observer.config.minimum_fit_r2 == observer_default_config().minimum_fit_r2;
}

int main(void) {
    int failures = 0;
    failures += require("normal remains stable", (run(SCENARIO_NORMAL).flags & DIAG_SPOT_UNSTABLE) == 0u);
    failures += require("leakage signature", (run(SCENARIO_LEAKAGE_DRIFT).flags & DIAG_LEAKAGE_ASSOCIATED) != 0u);
    failures += require("emission signature", (run(SCENARIO_EMISSION_OSCILLATION).flags & DIAG_EMISSION_ASSOCIATED) != 0u);
    failures += require("kV signature", (run(SCENARIO_KV_RIPPLE).flags & DIAG_KV_ASSOCIATED) != 0u);
    failures += require("thermal signature", (run(SCENARIO_THERMAL_DRIFT).flags & DIAG_THERMAL_ASSOCIATED) != 0u);
    failures += require("optical fallback", (run(SCENARIO_OPTICAL_ONLY).flags & DIAG_OPTICAL_OR_MECHANICAL) != 0u);
    failures += require("bad fit rejected before rolling window", test_quality_gate());
    failures += require("non-finite sample rejected", test_nonfinite_gate());
    failures += require("invalid configuration sanitized", test_config_sanitization());
    return failures ? 1 : 0;
}
