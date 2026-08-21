#include <cstdlib>
#include <cstring>
#include <iocsh.h>
#include <epicsExit.h>

int main(int argc, char *argv[]) {
    if (argc >= 2) {
        iocsh(argv[1]);
        epicsExit(0);
        return EXIT_SUCCESS;
    }
    iocsh(nullptr);
    epicsExit(0);
    return EXIT_SUCCESS;
}
