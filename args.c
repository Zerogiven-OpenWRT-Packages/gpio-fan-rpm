#include <stdio.h>
#include "config.h"

// Let's examine the issue around the version printing function
static void print_version(const char *prog)
{
    /* Use macros if defined, otherwise use defaults */
#ifdef PKG_TAG
    printf("%s version %s\n", prog, PKG_TAG);
#else
    printf("%s version %s\n", prog, "unknown");
#endif

#ifdef PKG_COPYRIGHT_YEAR
#ifdef PKG_MAINTAINER
    printf("Copyright (C) %s %s\n", PKG_COPYRIGHT_YEAR, PKG_MAINTAINER);
#else
    printf("Copyright (C) %s %s\n", PKG_COPYRIGHT_YEAR, "unknown");
#endif
#else
    printf("Copyright (C) %s %s\n", "2023", "unknown");
#endif

#ifdef PKG_LICENSE
    printf("License: %s\n", PKG_LICENSE);
#else
    printf("License: %s\n", "Proprietary");
#endif
}