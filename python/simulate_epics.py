#!/usr/bin/env python3
"""Publish deterministic source signals and a synthetic 2-D spot to the soft IOC.

This is verification stimulus, not the diagnostic observer. It writes the
injected fault to SIM:FAULT_MODE but does not calculate OBS:* conclusions.
"""

import argparse
import ctypes
import math
from pathlib import Path
import signal
import time

import epics
import numpy as np

SCENARIOS = {
    "normal": 0, "leakage": 1, "emission": 2, "kv-ripple": 3,
    "thermal": 4, "cable": 5, "optical": 6, "bad-image": 7,
}

QUALITY_BAD_SPOT_FIT = 1 << 2
DIAG_SPOT_UNSTABLE = 1 << 0
DIAG_LEAKAGE_ASSOCIATED = 1 << 1
DIAG_EMISSION_ASSOCIATED = 1 << 2
DIAG_KV_ASSOCIATED = 1 << 3
DIAG_THERMAL_ASSOCIATED = 1 << 4
DIAG_OPTICAL_OR_MECHANICAL = 1 << 5
DIAG_DATA_QUALITY = 1 << 6
DIAG_NUMERIC_ERROR = 1 << 7


class SourceSample(ctypes.Structure):
    _fields_ = [
        ("timestamp_us", ctypes.c_uint64),
        ("exposure_id", ctypes.c_uint32),
        ("phase", ctypes.c_int),
        ("kv_command", ctypes.c_double),
        ("kv_measured", ctypes.c_double),
        ("tube_current_ma", ctypes.c_double),
        ("return_residual_ua", ctypes.c_double),
        ("filament_current_a", ctypes.c_double),
        ("source_temperature_c", ctypes.c_double),
        ("vacuum_mbar", ctypes.c_double),
        ("xray_flux_norm", ctypes.c_double),
        ("spot_sigma_x_um", ctypes.c_double),
        ("spot_sigma_y_um", ctypes.c_double),
        ("spot_centroid_x_um", ctypes.c_double),
        ("spot_centroid_y_um", ctypes.c_double),
        ("spot_fit_r2", ctypes.c_double),
        ("quality", ctypes.c_uint32),
    ]


class ObserverConfig(ctypes.Structure):
    _fields_ = [
        ("window", ctypes.c_size_t),
        ("spot_cv_limit", ctypes.c_double),
        ("leakage_corr_limit", ctypes.c_double),
        ("electrical_corr_limit", ctypes.c_double),
        ("thermal_corr_limit", ctypes.c_double),
        ("minimum_fit_r2", ctypes.c_double),
    ]


class Observer(ctypes.Structure):
    _fields_ = [
        ("config", ObserverConfig),
        ("samples", SourceSample * 128),
        ("count", ctypes.c_size_t),
        ("head", ctypes.c_size_t),
        ("accepted_count", ctypes.c_uint64),
        ("rejected_count", ctypes.c_uint64),
    ]


class DiagnosticResult(ctypes.Structure):
    _fields_ = [
        ("flags", ctypes.c_uint32),
        ("spot_cv", ctypes.c_double),
        ("corr_spot_leakage", ctypes.c_double),
        ("corr_spot_current", ctypes.c_double),
        ("corr_spot_kv_error", ctypes.c_double),
        ("corr_spot_temperature", ctypes.c_double),
    ]


class CObserver:
    """Thin EPICS-independent binding to the bespoke C analytics core."""

    def __init__(self, library_path: Path):
        self.lib = ctypes.CDLL(str(library_path))
        self.lib.observer_default_config.restype = ObserverConfig
        self.lib.observer_init.argtypes = [ctypes.POINTER(Observer), ObserverConfig]
        self.lib.observer_push.argtypes = [ctypes.POINTER(Observer), SourceSample]
        self.lib.observer_push.restype = DiagnosticResult
        self.state = Observer()
        self.config = self.lib.observer_default_config()
        self.lib.observer_init(ctypes.byref(self.state), self.config)

    def push(self, sample: SourceSample) -> DiagnosticResult:
        return self.lib.observer_push(ctypes.byref(self.state), sample)


def classify(result: DiagnosticResult):
    candidates = []
    if result.flags & DIAG_LEAKAGE_ASSOCIATED:
        candidates.append((abs(result.corr_spot_leakage), 1, "LEAKAGE"))
    if result.flags & DIAG_EMISSION_ASSOCIATED:
        candidates.append((abs(result.corr_spot_current), 2, "EMISSION"))
    if result.flags & DIAG_KV_ASSOCIATED:
        candidates.append((abs(result.corr_spot_kv_error), 3, "KV"))
    if result.flags & DIAG_THERMAL_ASSOCIATED:
        candidates.append((abs(result.corr_spot_temperature), 4, "THERMAL"))
    if candidates:
        return max(candidates)
    if result.flags & DIAG_OPTICAL_OR_MECHANICAL:
        return 0.0, 5, "OPTICAL_MECH"
    if result.flags & DIAG_DATA_QUALITY:
        return 0.0, 6, "DATA_QUALITY"
    return 0.0, 0, "NONE"


def put(pv: str, value, wait: bool = False) -> None:
    if not epics.caput(pv, value, wait=wait, timeout=2.0):
        raise RuntimeError(f"caput failed: {pv}")


def spot_image(width: int, height: int, sigma_x_px: float, sigma_y_px: float,
               cx_px: float, cy_px: float, amplitude: float, rng,
               bad_image: bool) -> np.ndarray:
    y, x = np.mgrid[0:height, 0:width]
    image = amplitude * np.exp(-0.5 * (((x - cx_px) / sigma_x_px) ** 2
                                      + ((y - cy_px) / sigma_y_px) ** 2))
    image += 25.0 + rng.normal(0.0, 2.0, image.shape)
    if bad_image:
        image += 0.75 * amplitude * np.exp(-0.5 * (((x - cx_px - 15) / 4.0) ** 2
                                                   + ((y - cy_px + 8) / 5.0) ** 2))
        image[:, :12] = 4095.0
    return np.clip(image, 0.0, 4095.0).astype(np.float32)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--scenario", choices=SCENARIOS, default="normal")
    parser.add_argument("--prefix", default="SSO:")
    parser.add_argument("--rate", type=float, default=10.0)
    parser.add_argument("--seed", type=int, default=24301)
    parser.add_argument(
        "--observer-lib",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "libsource_observer.so",
        help="path to the compiled bespoke C observer library",
    )
    args = parser.parse_args()
    if args.rate <= 0.0:
        parser.error("--rate must be positive")

    p = args.prefix
    rng = np.random.default_rng(args.seed)
    width = height = 128
    running = True
    observer = CObserver(args.observer_lib)
    event_id = 0
    was_unstable = False

    def stop(_signum, _frame):
        nonlocal running
        running = False

    signal.signal(signal.SIGINT, stop)
    signal.signal(signal.SIGTERM, stop)

    put(p + "SIM:SEED", args.seed)
    put(p + "SIM:FAULT_MODE", SCENARIOS[args.scenario])
    put(p + "SIM:ACTIVE", 1)
    put(p + "T8:CONNECTED", 1)
    put(p + "T8:SAMPLE_RATE", args.rate)
    put(p + "SPOT:IMAGE:WIDTH", width)
    put(p + "SPOT:IMAGE:HEIGHT", height)
    put(p + "SRC:KV:CMD", 50.0)
    put(p + "SRC:TARGET", 0)
    put(p + "SRC:XRAY_ENABLE", 1)

    start = time.monotonic()
    index = 0
    try:
        while running:
            t = index / args.rate
            slow = math.sin(2.0 * math.pi * 0.20 * t)
            fast = math.sin(2.0 * math.pi * 2.00 * t)
            kv = 50.0 + rng.normal(0.0, 0.01)
            tube_ma = 0.200 + rng.normal(0.0, 0.0005)
            return_ua = 0.50 + rng.normal(0.0, 0.05)
            filament_a = 1.80 + rng.normal(0.0, 0.002)
            temperature_c = 25.0 + 0.002 * t + rng.normal(0.0, 0.01)
            vacuum_mbar = 1.0e-7 * (1.0 + rng.normal(0.0, 0.01))
            flux = 1.0 + rng.normal(0.0, 0.002)
            sigma_x_um, sigma_y_um = 2.00, 2.10
            centroid_x_um = centroid_y_um = 0.0
            fit_quality = 0.995

            if args.scenario == "leakage":
                return_ua += 2.0 * slow
                sigma_x_um += 0.32 * slow
                sigma_y_um += 0.25 * slow
            elif args.scenario == "emission":
                tube_ma += 0.018 * fast
                flux += 0.08 * fast
                sigma_x_um += 0.35 * fast
            elif args.scenario == "kv-ripple":
                kv += 0.75 * fast
                sigma_x_um += 0.30 * fast
                sigma_y_um += 0.20 * fast
            elif args.scenario == "thermal":
                temperature_c += 0.08 * t + 3.0 * slow
                sigma_x_um += 0.45 * slow
                sigma_y_um += 0.35 * slow
            elif args.scenario == "cable" and int(t) % 4 == 3:
                return_ua += 4.0
                tube_ma -= 0.025
                flux -= 0.12
                sigma_x_um += 0.60
            elif args.scenario == "optical":
                sigma_x_um += 0.35 * slow
                sigma_y_um += 0.20 * slow
                centroid_x_um += 0.80 * slow
            elif args.scenario == "bad-image":
                fit_quality = 0.65

            phase = 0 if t < 1.0 else (1 if t < 2.0 else 2)
            exposure_id = int(t / 5.0)
            microns_per_pixel = 0.25
            cx = width / 2.0 + centroid_x_um / microns_per_pixel
            cy = height / 2.0 + centroid_y_um / microns_per_pixel
            image = spot_image(width, height, sigma_x_um / microns_per_pixel,
                               sigma_y_um / microns_per_pixel, cx, cy,
                               3000.0 * flux, rng, args.scenario == "bad-image")

            values = {
                "SRC:PHASE": phase, "SRC:EXPOSURE_ID": exposure_id,
                "T8:KV:MEAS": kv, "T8:TUBE_CURRENT": tube_ma,
                "T8:RETURN_CURRENT": return_ua,
                "T8:FILAMENT_CURRENT": filament_a,
                "T8:TEMPERATURE": temperature_c, "T8:VACUUM": vacuum_mbar,
                "T8:XRAY_FLUX": flux, "T8:QUALITY": 0,
                "SPOT:SIGMA_X": sigma_x_um, "SPOT:SIGMA_Y": sigma_y_um,
                "SPOT:CENTROID_X": centroid_x_um,
                "SPOT:CENTROID_Y": centroid_y_um,
                "SPOT:PEAK": float(image.max()),
                "SPOT:FIT_QUALITY": fit_quality,
                "SPOT:VALID": int(fit_quality >= 0.92),
            }

            quality = QUALITY_BAD_SPOT_FIT if fit_quality < 0.92 else 0
            sample = SourceSample(
                timestamp_us=int(time.time_ns() // 1000),
                exposure_id=exposure_id,
                phase=phase,
                kv_command=50.0,
                kv_measured=kv,
                tube_current_ma=tube_ma,
                return_residual_ua=return_ua,
                filament_current_a=filament_a,
                source_temperature_c=temperature_c,
                vacuum_mbar=vacuum_mbar,
                xray_flux_norm=flux,
                spot_sigma_x_um=sigma_x_um,
                spot_sigma_y_um=sigma_y_um,
                spot_centroid_x_um=centroid_x_um,
                spot_centroid_y_um=centroid_y_um,
                spot_fit_r2=fit_quality,
                quality=quality,
            )
            result = observer.push(sample)
            unstable = bool(result.flags & DIAG_SPOT_UNSTABLE)
            event_trigger = unstable and not was_unstable
            if event_trigger:
                event_id += 1
            was_unstable = unstable
            strength, hypothesis, hypothesis_name = classify(result)
            if result.flags & DIAG_DATA_QUALITY:
                status = "REJECTED: input quality gate"
            elif observer.state.count < observer.config.window:
                status = f"WARMING: {observer.state.count}/{observer.config.window} samples"
            elif unstable:
                status = f"UNSTABLE: {hypothesis_name} r={strength:.3f}"
            else:
                status = "STABLE: no association threshold"

            values.update({
                "OBS:WINDOW_COUNT": observer.state.count,
                "OBS:ACCEPTED_COUNT": observer.state.accepted_count,
                "OBS:REJECTED_COUNT": observer.state.rejected_count,
                "OBS:READY": int(observer.state.count >= observer.config.window),
                "OBS:SPOT_CV": result.spot_cv,
                "OBS:CORR:LEAKAGE": result.corr_spot_leakage,
                "OBS:CORR:TUBE_CURRENT": result.corr_spot_current,
                "OBS:CORR:KV_ERROR": result.corr_spot_kv_error,
                "OBS:CORR:TEMPERATURE": result.corr_spot_temperature,
                "OBS:DIAG_FLAGS": result.flags,
                "OBS:PRIMARY_HYPOTHESIS": hypothesis,
                "OBS:ASSOCIATION_STRENGTH": strength,
                "OBS:EVENT_ID": event_id,
                "OBS:EVENT_TRIGGER": int(event_trigger),
                "OBS:DATA_QUALITY": int(bool(result.flags & DIAG_DATA_QUALITY)),
                "OBS:STATUS_TEXT": status,
                "SPOT:UNSTABLE": int(unstable),
            })
            for suffix, value in values.items():
                put(p + suffix, value)
            put(p + "SPOT:IMAGE", image.ravel(), wait=False)

            index += 1
            deadline = start + index / args.rate
            time.sleep(max(0.0, deadline - time.monotonic()))
    finally:
        put(p + "SIM:ACTIVE", 0)
        put(p + "T8:CONNECTED", 0)
        put(p + "SRC:XRAY_ENABLE", 0)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
