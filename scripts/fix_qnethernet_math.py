# Pre-build script.
#
# The QNEthernet IEEE 1588 fork (pinned in platformio.ini) calls round() in
# src/lwip_t41.c without including <math.h>. GCC 14+ rejects implicit
# function declarations, and silencing the error would make round() return
# garbage (implicit int return). Force-include <math.h> for C sources only.
#
# Remove this once the fork includes <math.h> itself.

Import("env")

env.Append(CFLAGS=["-include", "math.h"])
