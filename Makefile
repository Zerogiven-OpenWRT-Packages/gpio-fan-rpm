include $(TOPDIR)/rules.mk

PKG_NAME    := gpio-fan-rpm
PKG_VERSION := 1.0.0
PKG_RELEASE := 1

PKG_SOURCE_PROTO := git
PKG_SOURCE_URL := https://github.com/CSoellinger/gpio-fan-rpm.git
PKG_SOURCE_VERSION := main
PKG_MIRROR_HASH := skip

PKG_MAINTAINER     := CSoellinger
PKG_LICENSE        := GPL
PKG_LICENSE_FILES  := LICENSE
PKG_COPYRIGHT_YEAR := $(shell date +%Y)

PKG_BUILD_DEPENDS := libgpiod libjson-c

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=GPIO fan RPM measurement for OpenWRT
  DEPENDS:=+libgpiod +libjson-c
  PKGARCH:=all
endef

define Package/$(PKG_NAME)/description
  Tool for measuring fan RPM using GPIO pins on OpenWRT devices.
  Compatible with both libgpiod v1 (OpenWRT 23.05) and v2 (OpenWRT 24.10).
endef

# Enable pthread and link required libraries
TARGET_CFLAGS += -Wall -Wextra -pthread $(FPIC) \
  -DPKG_TAG=\"$(PKG_VERSION)-r$(PKG_RELEASE)\" \
  -DPKG_MAINTAINER=\"$(PKG_MAINTAINER)\" \
  -DPKG_LICENSE=\"$(PKG_LICENSE)\" \
  -DPKG_COPYRIGHT_YEAR=\"$(PKG_COPYRIGHT_YEAR)\"

TARGET_LDFLAGS += -pthread 

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR)/src \
		CROSS="$(TARGET_CROSS)"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/src/gpio-fan-rpm $(1)/usr/bin/
endef

$(eval $(call BuildPackage,$(PKG_NAME)))
