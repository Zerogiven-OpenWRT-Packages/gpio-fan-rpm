#ifndef CONFIG_H
#define CONFIG_H

/* Default values for package info if not provided by build system */
#ifndef PKG_TAG
#define PKG_TAG "dev-build"
#endif

#ifndef PKG_MAINTAINER
#define PKG_MAINTAINER "Unknown"
#endif

#ifndef PKG_LICENSE
#define PKG_LICENSE "Proprietary"
#endif

#ifndef PKG_COPYRIGHT_YEAR
#define PKG_COPYRIGHT_YEAR "2023"
#endif

#endif /* CONFIG_H */