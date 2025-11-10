#
# Copyright (C) 2025 CSoellinger
# This is free software, licensed under the GNU General Public License v3.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME    := gpio-fan-rpm
PKG_VERSION := 1.0.0
PKG_RELEASE := 1

PKG_MAINTAINER     := CSoellinger
PKG_LICENSE        := GPL-3.0-only
PKG_LICENSE_FILES  := LICENSE
PKG_COPYRIGHT_YEAR := $(shell date +%Y)

PKG_BUILD_DEPENDS:=libgpiod libjson-c

# Detect libgpiod version by checking header files
# This is the most reliable method for OpenWRT builds
LIBGPIOD_V2_HEADER := $(shell test -f $(STAGING_DIR)/usr/include/gpiod.h && grep -q "gpiod_request_config" $(STAGING_DIR)/usr/include/gpiod.h && echo "yes" || echo "no")
ifeq ($(LIBGPIOD_V2_HEADER),yes)
  LIBGPIOD_VERSION := v2
  TARGET_CFLAGS += -DLIBGPIOD_V2
  $(info Detected libgpiod v2 - using LIBGPIOD_V2)
else
  LIBGPIOD_VERSION := v1
  TARGET_CFLAGS += -DLIBGPIOD_V1
  $(info Detected libgpiod v1 - using LIBGPIOD_V1)
endif

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=GPIO Fan RPM Monitor
  URL:=https://github.com/Zerogiven-OpenWRT-Packages/gpio-fan-rpm
  MAINTAINER:=$(PKG_MAINTAINER)
  DEPENDS:=+libgpiod +libjson-c +libpthread +librt +libubox +libuci
endef

define Package/$(PKG_NAME)/description
  High-precision command-line utility for measuring fan RPM using GPIO pins on OpenWRT devices.
  Compatible with both OpenWRT 23.05 (libgpiod v1) and 24.10 (libgpiod v2).
  
  Features:
  - Edge-event-based RPM detection using libgpiod
  - Multithreaded measurement for multiple GPIOs in parallel
  - Warm-up phase to avoid initial inaccuracies
  - Watch mode for continuous monitoring
  - JSON, numeric and collectd-compatible output
  - Auto-detection of GPIO chip if not specified
endef

# Enable pthread and link required libraries
TARGET_CFLAGS += -Wall -Wextra -pthread $(FPIC) \
  -DPKG_TAG=\"$(PKG_VERSION)-r$(PKG_RELEASE)\" \
  -DPKG_MAINTAINER=\"$(PKG_MAINTAINER)\" \
  -DPKG_LICENSE=\"$(PKG_LICENSE)\" \
  -DPKG_COPYRIGHT_YEAR=\"$(PKG_COPYRIGHT_YEAR)\"

# Add libgpiod version to CFLAGS
TARGET_CFLAGS += -DLIBGPIOD_VERSION=\"$(LIBGPIOD_VERSION)\"
TARGET_LDFLAGS += -pthread -lrt -luci -lubox

# Pass library search paths explicitly using standard OpenWRT variables
define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib" \
		LIB="-lgpiod -ljson-c -lpthread -lrt -luci -lubox"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gpio-fan-rpm $(1)/usr/sbin/gpio-fan-rpm
	
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./files/etc/config/gpio-fan-rpm $(1)/etc/config/
endef

$(eval $(call BuildPackage,$(PKG_NAME))) 