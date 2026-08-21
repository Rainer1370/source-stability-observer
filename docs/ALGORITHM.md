# Observer algorithm

## File map

- `include/source_observer.h`: the stable contract—scenarios, sample schema, quality
  and diagnostic bit masks, configuration, fixed-size observer state, and API.
- `src/simulator.c`: deterministic synthetic sensor and spot data. A linear
  congruential generator makes noise repeatable; each switch branch injects one fault.
- `src/observer.c`: bounded circular buffer, numerically stable streaming variance,
  correlation, decision logic, and diagnostic text.
- `src/main.c`: thin CLI that keeps orchestration separate from analysis.
- `src/input_labjack_t8.c`: optional LJM stream seam; deliberately absent from the
  default build so no hardware or vendor library is required.
- `tests/test_observer.c`: requirement-style tests over complete scenarios.

There is no heap allocation, recursion, global mutable state, or blocking I/O inside
the observer. The maximum work per update is bounded by `OBSERVER_MAX_WINDOW`.

## Decision core

```c
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
```

1. The first line gates attribution: do not diagnose causes when the spot is stable.
2. The next line records the observed symptom separately from hypotheses.
3–4. Absolute leakage correlation catches positive or inverse association.
5–6. Current association points toward emission/load behavior.
7–8. The algorithm correlates spot with **kV error**, not absolute kV.
9–10. Temperature association creates a thermal test branch.
11–13. A bit mask asks whether *any* instrumented physical domain explains the event.
14. If none does, do not say “mystery solved”; route to optics, mechanics, or estimator.
15. Return all compatible flags—faults need not be mutually exclusive.
