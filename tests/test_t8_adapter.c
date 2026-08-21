#include "t8_source.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition); ++failures; \
} } while (0)

int main(void) {
    t8_source_t source = {.handle = -1, .scans_per_read = 100,
                          .channel_count = T8_SOURCE_CHANNEL_COUNT};
    double values[800] = {0};
    int device_backlog = 0, library_backlog = 0;
    CHECK(t8_required_values(&source) == 800u);
    CHECK(t8_read(&source, values, 800u, &device_backlog, &library_backlog) ==
          T8_SOURCE_ERR_NOT_OPEN);
    source.stream_started = 1;
    CHECK(t8_read(&source, values, 799u, &device_backlog, &library_backlog) ==
          T8_SOURCE_ERR_BUFFER_TOO_SMALL);
    CHECK(t8_read(NULL, values, 800u, &device_backlog, &library_backlog) ==
          T8_SOURCE_ERR_ARGUMENT);
    CHECK(t8_close(&source) == T8_SOURCE_OK);
    CHECK(t8_close(&source) == T8_SOURCE_OK);
    CHECK(source.handle == -1 && source.stream_started == 0);
    CHECK(strstr(t8_source_error_text(T8_SOURCE_ERR_BUFFER_TOO_SMALL), "buffer") != NULL);
    if (failures == 0) puts("PASS: T8 adapter contract");
    return failures == 0 ? 0 : 1;
}
