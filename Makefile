CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
CPPFLAGS := -Iinclude
LDLIBS := -lm
CORE := src/observer.c src/simulator.c

.PHONY: all test sanitize demo clean
all: observer_demo test_observer test_t8_adapter test_exposure_sync libsource_observer.so
observer_demo: src/main.c $(CORE) include/source_observer.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ src/main.c $(CORE) $(LDLIBS)
test_observer: tests/test_observer.c $(CORE) include/source_observer.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_observer.c $(CORE) $(LDLIBS)
test_t8_adapter: tests/test_t8_adapter.c src/input_labjack_t8.c include/t8_source.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_t8_adapter.c src/input_labjack_t8.c
test_exposure_sync: tests/test_exposure_sync.c src/exposure_sync.c include/exposure_sync.h include/source_observer.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ tests/test_exposure_sync.c src/exposure_sync.c $(LDLIBS)
test: test_observer test_t8_adapter test_exposure_sync
	./test_observer
	./test_t8_adapter
	./test_exposure_sync
sanitize:
	$(MAKE) clean
	ASAN_OPTIONS=detect_leaks=0 $(MAKE) CFLAGS='-std=c11 -O1 -g -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined' test
libsource_observer.so: src/observer.c include/source_observer.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -shared -o $@ src/observer.c $(LDLIBS)
demo: observer_demo
	./observer_demo leakage 1000 > leakage_demo.csv
clean:
	$(RM) observer_demo test_observer test_t8_adapter test_exposure_sync libsource_observer.so leakage_demo.csv
