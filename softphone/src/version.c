#include "version.h"
#include "shell.h"
#include <stdio.h>

const char* DEV_NAME = "STM32F429 SIP";

const char* APP_VERSION = "1.1.0";

const char* BUILD_TIMESTAMP = __DATE__", "__TIME__;

static enum shell_error sh_version(int argc, char ** argv) {
    printf("Version %s, built %s\n", APP_VERSION, BUILD_TIMESTAMP);
    return SHELL_ERR_NONE;
}

static __attribute__((constructor)) void registerCmd(void)
{
	shell_add("version", sh_version, "Show application version");
}
