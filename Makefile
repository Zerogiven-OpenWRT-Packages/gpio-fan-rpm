include $(TOPDIR)/rules.mk

PKG_NAME    := gpio-fan-rpm
PKG_VERSION := 1.0.0
PKG_RELEASE := 1

PKG_MAINTAINER     := CSoellinger
PKG_LICENSE        := GPL
PKG_COPYRIGHT_YEAR := $(shell date +%Y)

PKG_BUILD_DEPENDS := libgpiod libjson-c
PKG_FIXUP         := autoreconf

include $(INCLUDE_DIR)/package.mk
include release.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=GPIO Fan RPM Monitor
  DEPENDS:=+libgpiod +libjson-c +libpthread
  PKGARCH:=all
endef

define Package/$(PKG_NAME)/description
  gpio-fan-rpm measures fan speed (RPM) by counting GPIO edge events.
  It uses libgpiod v2 with multithreaded edge detection and supports multiple GPIOs,
  JSON/numeric/collectd outputs, and a warm-up phase for stable readings.
endef

# Enable pthread and link required libraries
TARGET_CFLAGS += -Wall -Wextra -pthread $(FPIC) \
  -DPKG_TAG=\"$(PKG_VERSION)-r$(PKG_RELEASE)\" \
  -DPKG_MAINTAINER=\"$(PKG_MAINTAINER)\" \
  -DPKG_LICENSE=\"$(PKG_LICENSE)\" \
  -DPKG_COPYRIGHT_YEAR=\"$(PKG_COPYRIGHT_YEAR)\"
TARGET_LDFLAGS += -pthread
TARGET_LIBS := -ljson-c -lgpiod

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)" \
		LIBS="$(TARGET_LIBS)"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gpio-fan-rpm $(1)/usr/sbin/gpio-fan-rpm
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
