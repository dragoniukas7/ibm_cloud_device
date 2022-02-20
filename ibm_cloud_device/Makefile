include $(TOPDIR)/rules.mk

PKG_NAME:=ibm_cloud_device
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/ibm_cloud_device
	DEPENDS:=+lib_ibm_cloud +libopenssl +ubox
	CATEGORY:=Base system
	TITLE:=ibm_cloud_device
endef

define Package/ibm_cloud_device/description
	This is an example app package
endef

define Package/ibm_cloud_device/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ibm_cloud_device $(1)/usr/bin
	$(INSTALL_BIN) ./files/ibm_cloud_device.init $(1)/etc/init.d/ibm_cloud_device
	$(INSTALL_CONF) ./files/ibm_cloud_device.config $(1)/etc/config/ibm_cloud_device
endef

$(eval $(call BuildPackage,ibm_cloud_device))