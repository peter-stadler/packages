include $(TOPDIR)/rules.mk

PKG_NAME:=pst
PKG_VERSION:=1.0
PKG_RELEASE:=1
PKG_MAINTAINER:=Peter Stadler <peter.stadler@student.uibk.ac.at>

include $(INCLUDE_DIR)/package.mk
include $(TOPDIR)/feeds/packages/lang/python/python3-package.mk

define Package/pst
  SECTION:=admin
  CATEGORY:=Administration
  TITLE:=Pst mods
  DEPENDS:=+block-mount +kmod-usb-ohci +kmod-usb-storage +kmod-fs-ext4
# +ds-lite \
# 	+luci-ssl-nginx +luci-app-opkg \
# 	+libustream-openssl +ca-bundle +ca-certificates +ddns-scripts
endef

define Package/pst/description
Pst mods
endef

Build/Compile:=

define Package/pst/install
	$(INSTALL_DIR) $(1)/root/
	$(INSTALL_DATA) ./files/root.profile $(1)/root/.profile
	$(INSTALL_DIR) $(1)/etc/uci-defaults
	$(INSTALL_BIN) ./files/66_pst-setup-add $(1)/etc/uci-defaults/
	$(INSTALL_DIR) $(1)/usr/bin/
	# $(INSTALL_BIN) ./files/nsupdate.info $(1)/usr/bin/
	$(INSTALL_BIN) ./files/pst-move $(1)/usr/bin/
	$(SED) "s#PYTHON3_PKG_DIR#$(PYTHON3_PKG_DIR)#g" $(1)/usr/bin/pst-move
	$(INSTALL_DIR) $(1)/add
endef

$(eval $(call BuildPackage,pst))
