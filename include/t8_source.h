#ifndef T8_SOURCE_H
#define T8_SOURCE_H

#include <stddef.h>

#define T8_SOURCE_CHANNEL_COUNT 8

enum {
    T8_SOURCE_OK = 0,
    T8_SOURCE_ERR_ARGUMENT = -1,
    T8_SOURCE_ERR_NOT_BUILT = -2,
    T8_SOURCE_ERR_NOT_OPEN = -3,
    T8_SOURCE_ERR_BUFFER_TOO_SMALL = -4
};

typedef struct {
    int handle;
    int scans_per_read;
    int channel_count;
    int stream_started;
} t8_source_t;

size_t t8_required_values(const t8_source_t *t8);
int t8_open(t8_source_t *t8, const char *identifier, int scans_per_read,
            double *scan_rate_hz);
int t8_read(t8_source_t *t8, double *interleaved_scans, size_t value_capacity,
            int *device_backlog, int *library_backlog);
int t8_close(t8_source_t *t8);
const char *t8_source_error_text(int error);

#endif
