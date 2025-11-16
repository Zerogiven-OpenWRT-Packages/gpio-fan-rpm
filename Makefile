include $(TOPDIR)/rules.mk

PKG_NAME:=gpio-fan-rpm
PKG_VERSION:=2.0.1
PKG_RELEASE:=1

PKG_MAINTAINER     := CSoellinger
PKG_LICENSE        := LGPL-3.0-or-later
PKG_LICENSE_FILES  := LICENSE

PKG_SOURCE:=v$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/CSoellinger/gpio-fan-rpm/archive/refs/tags/
PKG_HASH:=405347fa3c2fdfcf61af7957f93f42a54ef32a8e52cac42df45869ddeba7d4f5
PKG_INSTALL:=1

PKG_BUILD_DEPENDS:=libgpiod libjson-c

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += -DPKG_TAG=$(PKG_VERSION)-r$(PKG_RELEASE)

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=GPIO Fan RPM Monitor
  URL:=https://github.com/Zerogiven-OpenWRT-Packages/gpio-fan-rpm
  MAINTAINER:=$(PKG_MAINTAINER)
  DEPENDS:=+libgpiod +libjson-c +libpthread
endef

define Package/$(PKG_NAME)/description
  High-precision command-line utility for measuring fan RPM using GPIO pins on OpenWRT devices.

  Features:
  - Edge-event-based RPM detection using libgpiod
  - Multithreaded measurement for multiple GPIOs in parallel
  - Warm-up phase to avoid initial inaccuracies
  - Watch mode for continuous monitoring
  - JSON, numeric and collectd-compatible output
  - Auto-detection of GPIO chip if not specified
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/gpio-fan-rpm $(1)/usr/sbin/gpio-fan-rpm
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
