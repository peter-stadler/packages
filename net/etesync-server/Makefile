include $(TOPDIR)/rules.mk

PKG_NAME:=etesync-server
PKG_VERSION:=0.3.0
PKG_RELEASE:=1

PKG_SOURCE:=etesync-server-$(PKG_VERSION).tar.gz
PKG_SOURCE_URL:=https://github.com/etesync/server/archive/v$(PKG_VERSION)#
PKG_HASH:=d0728effa898a8b7afb4ce7439e0d0fd46bc819008925f21788d7e113435b579

PKG_LICENSE:=AGPL-3.0-only
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=Peter Stadler <peter.stadler@student.uibk.ac.at>

PKG_UNPACK=$(HOST_TAR) -C $(PKG_BUILD_DIR) --strip-components=1 -xzf $(DL_DIR)/$(PKG_SOURCE)

include $(INCLUDE_DIR)/package.mk
include ../../lang/python/python3-package.mk


define Package/etesync-server
	SECTION:=net
	CATEGORY:=Network
	SUBMENU:=Web Servers/Proxies
	TITLE:=End-to-End Encrypted Secure Data Sync
	URL:=https://www.etesync.com/
	DEPENDS:=+nginx-ssl +python3-django-etesync-journal \
		+python3-drf-nested-routers +python3-django-cors-headers \
		+python3-django-restframework +python3-django +python3-light
	USERID:=etesync=44312
	VARIANT:=python3
endef

define Package/etesync-uwsgi
	$(call Package/etesync-server)
	TITLE += using uWSGI.
	DEPENDS += +uwsgi +uwsgi-python3-plugin +uwsgi-syslog-plugin
endef

# define Package/etesync-gunicorn
# 	$(call Package/etesync-server)
# 	TITLE += using gunicorn.
# 	DEPENDS += +gunicorn3
# endef


define Package/etesync-server/description
	End-to-End Encrypted Secure Data Sync
endef

Package/etesync-uwsgi/description = $(Package/etesync-server/description) \
	using uWSGI.

# Package/etesync-gunicorn/description = $(Package/etesync-server/description) \
# 	using gunicorn.


define Py3Build/Compile
	$(INSTALL_DIR) $(PKG_INSTALL_DIR)/$(PYTHON3_PKG_DIR)
endef


define Py3Package/etesync-server/install
	$(INSTALL_DIR) $(1)/etc/config/
	$(INSTALL_CONF) ./files/etesync_server $(1)/etc/config/

	$(INSTALL_DIR) $(1)/usr/share/etesync-server/etesync_server/
	$(LN) /var/etc/etesync-server.ini $(1)/usr/share/etesync-server/

	$(INSTALL_BIN) $(PKG_BUILD_DIR)/manage.py $(1)/usr/share/etesync-server/
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/etesync_server/* \
		$(1)/usr/share/etesync-server/etesync_server/

	$(INSTALL_CONF) ./files/nginx.conf.template $(1)/usr/share/etesync-server/
ifneq ($(CONFIG_IPV6),y)
	$(SED) '/listen\s*\[/d' $(1)/usr/share/etesync-server/nginx.conf.template
endif

	$(INSTALL_DIR) $(1)/www/etesync/static/
	$(LN) /www/etesync/static $(1)/usr/share/etesync-server/static

	$(INSTALL_DIR) $(1)/etc/uci-defaults/
	$(CP) ./files/80_setup-etesync-server $(1)/etc/uci-defaults/

	$(INSTALL_DIR) $(1)/etc/nginx/conf.d/
	$(INSTALL_CONF) ./files/etesync.locations $(1)/etc/nginx/conf.d/
endef

define Py3Package/etesync-uwsgi/install
	$(call Py3Package/etesync-server/install, $(1))

	$(INSTALL_DIR) $(1)/etc/uwsgi/vassals/
	$(INSTALL_CONF) ./files/etesync-server.uwsgi \$(1)/etc/uwsgi/vassals/

	$(INSTALL_DIR) $(1)/etc/init.d/
	$(INSTALL_BIN) ./files/init.uwsgi $(1)/etc/init.d/etesync-server
endef

# define Py3Package/etesync-gunicorn/install
# 	$(call Py3Package/etesync-server/install, $(1))
#
# endef


define Package/etesync-server/postrm
#!/bin/sh
[ -n "$${IPKG_INSTROOT}" ] && exit 0
rmdir /usr/share/etesync-server/etesync_server
[ "$${PKG_UPGRADE}" == "1" ] && exit 0
rm /usr/share/etesync-server/static
rm -r /www/etesync/
exit 0
endef

Package/etesync-uwsgi/postrm=$(Package/etesync-server/postrm)

# Package/etesync-gunicorn/postrm=$(Package/etesync-server/postrm)


define Package/etesync-server/conffiles
/etc/config/etesync_server
/usr/share/etesync-server/nginx.conf.template
/etc/nginx/conf.d/etesync.locations
/etc/init.d/etesync-server
endef

define Package/etesync-uwsgi/conffiles
$(call Package/etesync-server/conffiles)
/etc/uwsgi/etesync-server.ini
endef

# define Package/etesync-gunicorn/conffiles
# $(call Package/etesync-server/conffiles)
#
# endef


$(eval $(call Py3Package,etesync-uwsgi))
$(eval $(call BuildPackage,etesync-uwsgi))
$(eval $(call BuildPackage,etesync-uwsgi-src))
# $(eval $(call Py3Package,etesync-gunicorn))
# $(eval $(call BuildPackage,etesync-gunicorn))
