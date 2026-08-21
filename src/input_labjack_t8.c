#include "t8_source.h"

#include <string.h>

#ifdef HAVE_LJM
#include <LabJackM.h>
#endif

size_t t8_required_values(const t8_source_t *source) {
    if (source == NULL || source->scans_per_read <= 0 || source->channel_count <= 0)
        return 0u;
    return (size_t)source->scans_per_read * (size_t)source->channel_count;
}

int t8_open(t8_source_t *source, const char *identifier, int scans_per_read,
            double *scan_rate_hz) {
    if (source == NULL || scan_rate_hz == NULL || *scan_rate_hz <= 0.0 ||
        scans_per_read <= 0)
        return T8_SOURCE_ERR_ARGUMENT;
    memset(source, 0, sizeof(*source));
    source->handle = -1;
    source->scans_per_read = scans_per_read;
    source->channel_count = T8_SOURCE_CHANNEL_COUNT;
#ifndef HAVE_LJM
    (void)identifier;
    return T8_SOURCE_ERR_NOT_BUILT;
#else
    static const char *const channels[T8_SOURCE_CHANNEL_COUNT] = {
        "AIN0", "AIN1", "AIN2", "AIN3", "AIN4", "AIN5", "AIN6", "AIN7"
    };
    int addresses[T8_SOURCE_CHANNEL_COUNT];
    int types[T8_SOURCE_CHANNEL_COUNT];
    const char *device_id = (identifier == NULL || identifier[0] == '\0') ? "ANY" : identifier;
    int error = LJM_OpenS("T8", "ETHERNET", device_id, &source->handle);
    if (error != LJME_NOERROR) return error;
    for (size_t i = 0u; i < T8_SOURCE_CHANNEL_COUNT; ++i) {
        error = LJM_NameToAddress(channels[i], &addresses[i], &types[i]);
        if (error != LJME_NOERROR) {
            (void)LJM_Close(source->handle);
            source->handle = -1;
            return error;
        }
    }
    error = LJM_eStreamStart(source->handle, scans_per_read,
                             T8_SOURCE_CHANNEL_COUNT, addresses, scan_rate_hz);
    if (error != LJME_NOERROR) {
        (void)LJM_Close(source->handle);
        source->handle = -1;
        return error;
    }
    source->stream_started = 1;
    return T8_SOURCE_OK;
#endif
}

int t8_read(t8_source_t *source, double *interleaved_scans, size_t value_capacity,
            int *device_backlog, int *library_backlog) {
    if (source == NULL || interleaved_scans == NULL || device_backlog == NULL ||
        library_backlog == NULL)
        return T8_SOURCE_ERR_ARGUMENT;
    if (!source->stream_started) return T8_SOURCE_ERR_NOT_OPEN;
    if (value_capacity < t8_required_values(source)) return T8_SOURCE_ERR_BUFFER_TOO_SMALL;
#ifndef HAVE_LJM
    return T8_SOURCE_ERR_NOT_BUILT;
#else
    return LJM_eStreamRead(source->handle, interleaved_scans, device_backlog,
                           library_backlog);
#endif
}

int t8_close(t8_source_t *source) {
    if (source == NULL) return T8_SOURCE_ERR_ARGUMENT;
    int result = T8_SOURCE_OK;
#ifdef HAVE_LJM
    if (source->stream_started && source->handle >= 0) {
        const int error = LJM_eStreamStop(source->handle);
        if (error != LJME_NOERROR) result = error;
    }
    if (source->handle >= 0) {
        const int error = LJM_Close(source->handle);
        if (result == T8_SOURCE_OK && error != LJME_NOERROR) result = error;
    }
#endif
    source->handle = -1;
    source->stream_started = 0;
    return result;
}

const char *t8_source_error_text(int error) {
    switch (error) {
        case T8_SOURCE_OK: return "ok";
        case T8_SOURCE_ERR_ARGUMENT: return "invalid T8 adapter argument";
        case T8_SOURCE_ERR_NOT_OPEN: return "T8 stream is not open";
        case T8_SOURCE_ERR_BUFFER_TOO_SMALL: return "T8 destination buffer is too small";
        case T8_SOURCE_ERR_NOT_BUILT: return "adapter built without LabJack LJM support";
        default: return "LabJack LJM error";
    }
}
