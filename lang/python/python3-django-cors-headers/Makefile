include $(TOPDIR)/rules.mk

PKG_NAME:=django-cors-headers
PKG_VERSION:=3.7.0
PKG_RELEASE:=1

PYPI_NAME:=django-cors-headers
PKG_HASH:=96069c4aaacace786a34ee7894ff680780ec2644e4268b31181044410fecd12e

PKG_LICENSE:=MIT
PKG_LICENSE_FILES:=LICENSE
PKG_MAINTAINER:=Peter Stadler <peter.stadler@student.uibk.ac.at>

include ../pypi.mk
include $(INCLUDE_DIR)/package.mk
include ../python3-package.mk

define Package/python3-django-cors-headers
  SUBMENU:=Python
  SECTION:=lang
  CATEGORY:=Languages
  TITLE:=Django application for handling the server headers required for Cross-Origin Resource Sharing (CORS).
  URL:=https://github.com/adamchainz/django-cors-headers
  DEPENDS:=+python3-django +python3-urllib +python3-light
endef

define Package/python3-django-cors-headers/description
  Django application for handling the server headers required for Cross-Origin Resource Sharing (CORS).
endef

$(eval $(call Py3Package,python3-django-cors-headers))
$(eval $(call BuildPackage,python3-django-cors-headers))
$(eval $(call BuildPackage,python3-django-cors-headers-src))
