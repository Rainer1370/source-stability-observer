#include <cstdlib>
#include <cstring>
#include <iocsh.h>
#include <epicsExit.h>

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        iocsh(argv[1]);
    }

    /* Keep the IOC shell alive after the startup script reaches EOF. */
    iocsh(nullptr);
    epicsExit(0);
    return EXIT_SUCCESS;
}
