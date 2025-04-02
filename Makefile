include $(TOPDIR)/rules.mk

PKG_NAME:=gpio-fan-rpm
PKG_VERSION:=1.0
PKG_RELEASE:=1

PKG_MAINTAINER:=YourName
PKG_LICENSE:=MIT

include $(INCLUDE_DIR)/package.mk

define Package/$(PKG_NAME)
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=GPIO Fan RPM Monitor
endef

define Package/$(PKG_NAME)/description
  This package installs a utility that measures fan RPM by counting falling
  edges on GPIO input pins using sysfs and poll(). It supports multiple GPIOs,
  numeric output for monitoring systems, and live updates.
endef

TARGET_CFLAGS += $(FPIC)

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) \
		CC="$(TARGET_CC)" \
		CFLAGS="$(TARGET_CFLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS)"
endef

define Package/$(PKG_NAME)/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/gpio-fan-rpm $(1)/usr/sbin/gpio-fan-rpm
endef

$(eval $(call BuildPackage,gpio-fan-rpm))
