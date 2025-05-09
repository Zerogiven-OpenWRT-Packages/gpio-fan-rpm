include $(TOPDIR)/rules.mk

PKG_NAME    := gpio-fan-rpm
PKG_VERSION := 1.0.0
PKG_RELEASE := 1

PKG_MAINTAINER     := CSoellinger
PKG_LICENSE        := GPL
PKG_COPYRIGHT_YEAR := $(shell date +%Y)

PKG_BUILD_DEPENDS := libgpiod libjson-c

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=GPIO Fan RPM Monitor
  DEPENDS:=+libgpiod +libjson-c +libpthread
  PKGARCH:=all
endef

define Package/$(PKG_NAME)/description
  gpio-fan-rpm measures fan speed (RPM) by counting GPIO edge events.
  It supports both libgpiod v1 and v2 APIs for compatibility across OpenWRT
  versions (23.05+ and 24.10+), with multithreaded edge detection and outputs
  in various formats (JSON, collectd, numeric).
endef

# Enable pthread and link required libraries
TARGET_CFLAGS += -Wall -Wextra -pthread $(FPIC) \
  -DPKG_TAG=\"$(PKG_VERSION)-r$(PKG_RELEASE)\" \
  -DPKG_MAINTAINER=\"$(PKG_MAINTAINER)\" \
  -DPKG_LICENSE=\"$(PKG_LICENSE)\" \
  -DPKG_COPYRIGHT_YEAR=\"$(PKG_COPYRIGHT_YEAR)\"

TARGET_LDFLAGS += -pthread 

# Pass library search paths explicitly using standard OpenWRT variables
define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS) -I$(STAGING_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) -L$(STAGING_DIR)/usr/lib" \
		LIB="-lgpiod -ljson-c -lpthread"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gpio-fan-rpm $(1)/usr/sbin/gpio-fan-rpm
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
