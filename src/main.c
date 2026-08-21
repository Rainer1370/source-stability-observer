#include "source_observer.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    scenario_t scenario = SCENARIO_NORMAL;
    if (argc > 1 && !scenario_parse(argv[1], &scenario)) {
        fprintf(stderr, "scenario: normal|leakage|emission|kv-ripple|thermal|cable|optical\n");
        return 2;
    }
    const size_t samples = argc > 2 ? (size_t)strtoul(argv[2], NULL, 10) : 1000u;
    simulator_t sim; observer_t observer;
    simulator_init(&sim, scenario, 100.0, 0x5EEDu);
    observer_init(&observer, observer_default_config());
    diagnostic_result_t result = {0};
    puts("time_s,scenario,kv_meas,tube_ma,return_ua,temp_c,flux,spot_x_um,spot_y_um,flags");
    for (size_t i = 0; i < samples; ++i) {
        const source_sample_t x = simulator_next(&sim);
        result = observer_push(&observer, x);
        char flags[160]; diagnostic_flags_text(result.flags, flags, sizeof(flags));
        printf("%.6f,%s,%.5f,%.6f,%.5f,%.4f,%.5f,%.5f,%.5f,%s\n",
               x.timestamp_us / 1e6, scenario_name(scenario), x.kv_measured,
               x.tube_current_ma, x.return_residual_ua, x.source_temperature_c,
               x.xray_flux_norm, x.spot_sigma_x_um, x.spot_sigma_y_um, flags);
    }
    char flags[160]; diagnostic_flags_text(result.flags, flags, sizeof(flags));
    fprintf(stderr, "%s: %s; spot CV=%.4f; correlations leak=%.3f current=%.3f kV=%.3f temp=%.3f\n",
            scenario_name(scenario), flags, result.spot_cv, result.corr_spot_leakage,
            result.corr_spot_current, result.corr_spot_kv_error, result.corr_spot_temperature);
    return 0;
}
