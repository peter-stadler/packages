#!/bin/sh
# This is a template copy it by: ./README.sh | xclip -selection c
# to https://openwrt.org/docs/guide-user/services/webserver/nginx#configuration

NGINX_UTIL="/usr/bin/nginx-util"

EXAMPLE_COM="example.com"

MSG="
/* Created by the following bash script that includes the source of some files:
 * https://github.com/openwrt/packages/net/nginx-util/files/README.sh
 */"

eval $("${NGINX_UTIL}" get_env)

code() {
    local file
    [ $# -gt 1 ] && file="$2" || file="$(basename "$1")"
    printf "<file nginx %s>\n%s</file>" "$1" "$(cat "${file}")";
}

ifConfEcho() {
    sed -nE "s/^\s*$1=\s*(\S*)\s*\\\\$/\n$2 \"\1\";/p" ../../nginx/Makefile;
}

cat <<EOF





===== Configuration =====${MSG}



The official Documentation contains a
[[https://docs.nginx.com/nginx/admin-guide/|Admin Guide]].
Here we will look at some often used configuration parts and how we handle them
at OpenWrt.
At different places there are references to the official
[[https://docs.nginx.com/nginx/technical-specs/|Technical Specs]]
for further reading.

**tl;dr:** When starting Nginx by ''/etc/init.d/nginx'', it creates its main
configuration dynamically based on a minimal template and the
[[docs:guide-user:base-system:uci|🡒UCI]] configuration.

The UCI ''/etc/config/nginx'' contains initially:
| ''config server '${LAN_NAME}''' | \
Default server for th LAN, which includes all ''${CONF_DIR}*.locations''. |
| ''config server '_redirect2ssl''' | \
Redirects inexistent URLs to HTTPS (installed only for Nginx with SSL). |

It enables also the ''${CONF_DIR}'' directory for further configuration:
| ''${CONF_DIR}\$NAME.conf'' | \
Is included in the main configuration. \
It is prioritized over a UCI ''config server '\$NAME' ''. |
| ''${LAN_LISTEN}'' | \
For including in HTTP servers to make them reachable from LAN. |
| ''${LAN_SSL_LISTEN}'' | \
For including in HTTPS servers to make them available on LAN. |

Setup configuration (for a server ''\$NAME''):
| ''$(basename ${NGINX_UTIL}) [${ADD_SSL_FCT}|del_ssl] \$NAME''  | \
Add/remove a self-signed certificate and corresponding directives. |
| ''uci add_list nginx.\$NAME.${LISTEN_LOCALLY}=…'' | \
Makes the server reachable from LAN. |
| ''uci set nginx.\$NAME.access_log='logd openwrt''' | \
Writes accesses to Openwrt’s \
[[docs:guide-user:base-system:log.essentials|🡒logd]]. |
| ''uci set nginx.\$NAME.error_log='logd' '' | \
Writes errors to Openwrt’s \
[[docs:guide-user:base-system:log.essentials|🡒logd]]. |
| ''uci [set|add_list] nginx.\$NAME.key='value' '' | \
Becomes a ''key value;'' directive if the //key// does not start with //uci_//. |
| ''uci set nginx.\$NAME=[disable|server]'' |\
Disable/enable inclusion in the dynamic conf.|
| ''uci set nginx.global.uci_enable=false'' | \
Use a custom ''${NGINX_CONF}'' rather than a dynamic conf. |



==== Basic ====${MSG}


We modify the configuration by changing servers saved in the UCI configuration
at ''/etc/config/nginx'' and/or by creating different configuration files in the
''${CONF_DIR}'' directory.
These files use the file extensions ''.locations'' and ''.conf'' (plus ''.crt''
and ''.key'' for Nginx with SSL).((
We can disable a single configuration file by giving it another extension, e.g.,
by adding ''.disabled''.))
For the new configuration to take effect, we must reload it by:

<code bash>service nginx reload</code>

For OpenWrt we use a special initial configuration, which is explained in the
section [[#openwrt_s_defaults|🡓OpenWrt’s Defaults]].
So, we can make a site available at a specific URL in the **LAN** by creating a
''.locations'' file in the directory ''${CONF_DIR}''.
Such a file consists just of some
[[https://nginx.org/en/docs/http/ngx_http_core_module.html#location|
location blocks]].
Under the latter link, you can find also the official documentation for all
available directives of the HTTP core of Nginx.
Look for //location// in the Context list.

The following example provides a simple template, see at the end for
different [[#locations_for_apps|🡓Locations for Apps]]((look for
[[https://github.com/search?utf8=%E2%9C%93&q=repo%3Aopenwrt%2Fpackages
+extension%3Alocations&type=Code&ref=advsearch&l=&l=|
other packages using a .locations file]], too.)):

<code nginx $(basename ${CONF_DIR})/example.locations>
location /ex/am/ple {
	access_log off; # default: not logging accesses.
	# access_log /proc/self/fd/1 openwrt; # use logd (init forwards stdout).
	# error_log stderr; # default: logging to logd (init forwards stderr).
	error_log /dev/null; # disable error logging after config file is read.
	# (state path of a file for access_log/error_log to the file instead.)
	index index.html;
}
# location /eg/static { … }
</code>

All location blocks in all ''.locations'' files must use different URLs,
since they are all included in the ''${LAN_NAME}'' server that is part of the
[[#openwrt_s_defaults|🡓OpenWrt’s Defaults]].((
We reserve the ''location /'' for making LuCI available under the root URL,
e.g. [[http://192.168.1.1/|192.168.1.1/]].
All other sites shouldn’t use the root ''location /'' without suffix.))
We should make other sites than LuCI available on the root URL of **other**
domain names only, e.g. on //www.example.com///.
In order to do that, we create [[#new_server_parts|🡓New Server Parts]] for all
domain names.
For Nginx with SSL we can also activate SSL thereby, see
[[#ssl_server_parts|🡓SSL Server Parts]].
We use such server parts also for publishing sites to the internet (WAN)
instead of making them available just locally (in the LAN).

Via ''${CONF_DIR}*.conf'' files we can add directives to the //http// part of
the configuration.
If you would change the configuration ''$(basename "${UCI_CONF}").template''
instead, it is not updated to new package's versions anymore.
Although it is not recommended, you can also disable the whole UCI config and
create your own ''${NGINX_CONF}''; then invoke:

<code bash>uci set nginx.global.uci_enable=false</code>



==== New Server Parts ====${MSG}


For making the router reachable from the WAN at a registered domain name,
it is not enough to give the name server the internet IP address of the router
(maybe updated automatically by a
[[docs:guide-user:services:ddns:client|🡒DDNS Client]]).
We also need to set up virtual hosting for this domain name by creating an
appropriate server section in ''/etc/config/nginx''
or in a ''${CONF_DIR}*.conf'' file, but which cannot be changed by UCI).
All such parts are included in the main configuration of OpenWrt
([[#openwrt_s_defaults|🡓OpenWrt’s Defaults]]).

In the server part, we state the domain as
[[https://nginx.org/en/docs/http/ngx_http_core_module.html#server_name|
server_name]].
The link points to the same document as for the location blocks in the
[[#basic|🡑Basic Configuration]]: the official documentation for all available
directives of the HTTP core of Nginx.
This time look for //server// in the Context list, too.
The server part should also contain similar location blocks as
++before.|
We can re-include a ''.locations'' file that is included in the server part for
the LAN by default.
Then the site is reachable under the same path at both domains, e.g., by
http://192.168.1.1/ex/am/ple as well as by http://example.com/ex/am/ple.
++

The [[#openwrt_s_defaults|🡓OpenWrt’s Defaults]] has a
''config server '${LAN_NAME}' '' containing a server part that listens on the
LAN address(es) and acts as //default_server//.
For making another domain name accessible in the LAN, too, the corresponding
server part must listen **explicitly** on the local IP address(es), cf. the
official documentation on
[[https://nginx.org/en/docs/http/request_processing.html|request_processing]].
We can do so by adding to the UCI config ''list ${LISTEN_LOCALLY} '\$PORT …' ''.
When starting Nginx, each of them is
++substituted| by
''listen \$IP:\$PORT …;'' directives with ''\$IP'' running through all LAN
addresses.
Hereby, we can replace ''…'' by any argument that is recognized by Nginx’s
[[https://nginx.org/en/docs/http/ngx_http_core_module.html#listen|listen]]
directive (NB: there can only be one //default_server// for each port)
++.

We can add more directives by invoking
''uci [set|add_list] nginx.${EXAMPLE_COM//./_}.key=value''.
If the //key// is not starting with //uci_//, it becomes a ''key value;''
++directive.|
Although the UCI config does not support nesting like Nginx, we can add a whole
block as //value//.
++

We cannot use dots in a //key// name other than in the //value//.
In the following example we replace the dot in //${EXAMPLE_COM}// by an
underscore for the UCI name of the server, but not for Nginx's //server_name//:

<code bash>
uci add nginx server &&
uci rename nginx.@server[-1]=${EXAMPLE_COM//./_} &&
uci add_list nginx.${EXAMPLE_COM//./_}.listen='80' &&
uci add_list nginx.${EXAMPLE_COM//./_}.listen='[::]:80' &&
uci add_list nginx.${EXAMPLE_COM//./_}.${LISTEN_LOCALLY}='80' &&
uci set nginx.${EXAMPLE_COM//./_}.server_name='${EXAMPLE_COM}' &&
uci add_list nginx.${EXAMPLE_COM//./_}.include=\
'$(basename ${CONF_DIR})/${EXAMPLE_COM}.listen'
# uci add_list nginx.${EXAMPLE_COM//./_}.location='/ { … }' \
# root location for this server.
</code>

We can disable respective re-enable this server again:

<code bash>
uci set nginx.${EXAMPLE_COM//./_}=disable # respective: \
uci set nginx.${EXAMPLE_COM//./_}=server
</code>

These changes are made in the RAM (and can be used until a reboot), we can save
them permanently by:

<code bash>uci commit nginx</code>

For creating a similar ''${CONF_DIR}${EXAMPLE_COM}.conf'', we can include the
file ''${LAN_LISTEN}'' that contains the listen directives for all LAN addresses
on the HTTP port 80 and is updated automatically:
/*, see
 * [[https://github.com/search?q=repo%3Aopenwrt%2Fpackages
 * +include+${LAN_LISTEN}+extension%3Aconf&type=Code|
 * such server parts of other packages]], too:
 */

<code nginx $(basename ${CONF_DIR})/${EXAMPLE_COM}.conf>
server {
	listen 80;
	listen [::]:80;
	include '${LAN_LISTEN}';
	server_name ${EXAMPLE_COM};
	include '$(basename ${CONF_DIR})/${EXAMPLE_COM}.locations';
	# location / { … } # root location for this server.
}
</code>

Hereby, we cannot change the arguments of the LAN listen directives—unlike when
using //${LISTEN_LOCALLY}// in the UCI config.



==== SSL Server Parts ====${MSG}


We can enable HTTPS for a domain if Nginx is installed with SSL support.
We need a SSL certificate as well as its key and add them by the directives
//ssl_certificate// respective //ssl_certificate_key// to the server part of the
domain.
The rest of the configuration is similar as for general
[[#new_server_parts|🡑New Server Parts]],
we only have to adjust the listen directives by adding the //ssl// parameter,
see the official documentation for
[[https://nginx.org/en/docs/http/configuring_https_servers.html|
configuring HTTPS servers]], too.

The official documentation of the SSL module contains an
[[https://nginx.org/en/docs/http/ngx_http_ssl_module.html#example|
example]], which includes some optimizations.
We can extend an existing UCI server section similarly, e.g., for the above
''config server '${EXAMPLE_COM//./_}' '' we invoke:

<code bash>
# Instead of 'del_list' the listen* entries, we could use '443 ssl' beforehand.
uci del_list nginx.${EXAMPLE_COM//./_}.listen='80' &&
uci del_list nginx.${EXAMPLE_COM//./_}.listen='[::]:80' &&
uci del_list nginx.${EXAMPLE_COM//./_}.${LISTEN_LOCALLY}='80' &&
uci add_list nginx.${EXAMPLE_COM//./_}.listen='443 ssl' &&
uci add_list nginx.${EXAMPLE_COM//./_}.listen='[::]:443 ssl' &&
uci add_list nginx.${EXAMPLE_COM//./_}.${LISTEN_LOCALLY}='443 ssl' &&
uci set nginx.${EXAMPLE_COM//./_}.ssl_certificate=\
'${CONF_DIR}${EXAMPLE_COM}.crt' &&
uci set nginx.${EXAMPLE_COM//./_}.ssl_certificate_key=\
'${CONF_DIR}${EXAMPLE_COM}.key' &&
uci set nginx.${EXAMPLE_COM//./_}.ssl_session_cache=\
'${SSL_SESSION_CACHE_ARG}' &&
uci set nginx.${EXAMPLE_COM//./_}.ssl_session_timeout=\
'${SSL_SESSION_TIMEOUT_ARG}' &&
uci commit nginx
</code>

If we want to make the server in ''${CONF_DIR}${EXAMPLE_COM}.conf''
available in the LAN via SSL, we can—additionally to adding the //ssl_*//
directives—include the file ''${LAN_SSL_LISTEN}'' (instead of
''$(basename "${LAN_LISTEN}")'').
This file contains the listen directives with the //ssl// parameter for
all LAN addresses on the HTTPS port 443 and is updated automatically.
/*, see also
 * [[https://github.com/search?q=repo%3Aopenwrt%2Fpackages
 * +include+${LAN_SSL_LISTEN}+extension%3Aconf&type=Code|
 * other packages providing SSL server parts]].
 */

The following command creates a **self-signed** SSL certificate and changes the
corresponding configuration:

<code bash>$(basename "${NGINX_UTIL}") ${ADD_SSL_FCT} ${EXAMPLE_COM//./_}</code>

  - If a ''$(basename "${CONF_DIR}")/${EXAMPLE_COM//./_}.conf'' file exists, it\
    adds //ssl_*// directives and changes the listen directives for the server.\
    Else it does that for the UCI server like in the example above.
  - Then, it checks if there is a certificate with key for the given name\
    that is valid for at least 13 months or tries to create a self-signed one.
  - When cron is activated, it installs a cron job for renewing the self-signed\
    certificate every year if needed, too. We can activate cron by: \
    <code bash>service cron enable && service cron start</code>

This can be undone by invoking:

<code bash>$(basename "${NGINX_UTIL}") del_ssl ${EXAMPLE_COM//./_}</code>

For creating a certificate and its key signed by Let’s Encrypt we can use
[[https://github.com/ndilieto/uacme|uacme]] or
[[https://github.com/Neilpang/acme.sh|acme.sh]], which are installed by:

<code bash>
opkg update && opkg install uacme #or: acme #and for LuCI: luci-app-acme
</code>

[[#openwrt_s_defaults|🡓OpenWrt’s Defaults]] include a UCI server for the LAN:
''config server '${LAN_NAME}' ''. It has //ssl_*// directives prepared for a
self-signed SSL certificate((Let’s Encrypt (and other CAs) cannot sign
certificates of a **local** server.))
that is created on the first start of Nginx.
Furthermore, there is also a UCI server named ''_redirect2ssl'' that redirects
all HTTP requests for inexistent URLs to HTTPS.



==== OpenWrt’s Defaults ====${MSG}


Since Nginx is compiled with these presets, we can pretend that the main
configuration will always contain the following directives
(though we can overwrite them):

<code nginx>$(ifConfEcho --pid-path pid)\
$(ifConfEcho --lock-path lock_file)\
$(ifConfEcho --error-log-path error_log)\
$(false && ifConfEcho --http-log-path access_log)\
$(ifConfEcho --http-proxy-temp-path proxy_temp_path)\
$(ifConfEcho --http-client-body-temp-path client_body_temp_path)\
$(ifConfEcho --http-fastcgi-temp-path fastcgi_temp_path)\
</code>

When starting or reloading the Nginx service, the ''/etc/init.d/nginx'' script
sets also the following directives
(so we cannot change them in the used configuration file):

<code nginx>
daemon off; # procd expects services to run in the foreground
worker_processes '\$NCPUS'; # NCPUS="\$(grep -c '^processor\s*:' /proc/cpuinfo)"
</code>

Then, it creates the main configuration ''$(basename "${UCI_CONF}")''
dynamically from the template:

$(code "${UCI_CONF}.template")

So, the access log is turned off by default and we can look at the error log
by ''logread'', as Nginx’s init script forwards stderr and stdout to the
[[docs:guide-user:base-system:log.essentials|🡒runtime log]].
We can set the //error_log// and //access_log// to files, where the log
messages are forwarded to instead (after the configuration is read).
And for redirecting the access log of a //server// or //location// to the logd,
too, we insert the following directive in the corresponding block:

<code nginx>	access_log /proc/self/fd/1 openwrt;</code>

If we setup a server through UCI, we can use the options //error_log// and/or
//access_log// with the path
++'logd'.|
When initializing the Nginx service, this special path is replaced by //stderr//
respective ///proc/self/fd/1// (which are forwarded to the runtime log).
++

For creating the configuration from the template shown above, the init.d script
replaces the comment ''#UCI_HTTP_CONFIG'' by all UCI servers.
For each server section in the the UCI configuration, it basically copies all
options into a Nginx //server { … }// part, in detail:
  * It replaces every ''list ${LISTEN_LOCALLY} '\$PORT …' '' by directives of\
  the form ''listen \$IP:\$PORT …;'', where ''\$IP'' runs through all LAN\
  addresses.
  * Other options starting with ''uci_'' are skipped. Currently there is only\
  the ''option ${MANAGE_SSL}=…'' in ++usage.| It is set to\
  //'self-signed'// when invoking ''$(basename ${NGINX_UTIL}) $ADD_SSL_FCT …''.\
  Then the corresponding certificate is re-newed if it is about to expire.\
  All those certificates are checked on the initialization of the Nginx service\
  and if Cron is available, it is deployed for checking them annually, too.++
  * All other lists or options of the form ''key='value' '' are written\
  one-to-one as ''key value;'' directives to the configuration file.\
  Just the path //logd// has a special meaning for the logging directives\
  (described in the previous paragraph).

The init.d script of Nginx uses the //$(basename ${NGINX_UTIL})// for creating
the configuration file
++in RAM.|
The main configuration ''${UCI_CONF}'' is a symbolic link to this place
(it is a dead link if the Nginx service is not running).
++

We could use a custom configuration created at ''${NGINX_CONF}'' instead of the
dynamic configuration, too.((
For using a custom configuration at ''${NGINX_CONF}'', we execute
<code bash>uci set nginx.global.uci_enable='false' </code>
Then the rest of the UCI config is ignored and init.d will not create the main
configuration dynamically from the template anymore.
For Nginx with SSL invoking
''$(basename ${NGINX_UTIL}) [add_ssl|del_ssl] \$FQDN''
will still try to change a server in ''$(basename "${CONF_DIR}")/\$FQDN.conf''
(this is less reliable than for a UCI config as it uses regular expressions, not
a complete parser for the Nginx configuration).))
This is not encouraged since you cannot setup servers using UCI anymore.
Rather, we can put custom configuration parts to ''.conf'' files in the
''${CONF_DIR}'' directory.
The main configuration pulls in all ''$(basename "${CONF_DIR}")/*.conf'' files
into the //http {…}// block behind the created UCI servers.

The initial UCI config is enabled and contains a server section for the LAN:

$(code "/etc/config/nginx" "config-nginx")

The LAN server pulls in all ''.locations'' files from the directory
''${CONF_DIR}''.
We can install the location parts of different sites there (see
[[#basic|🡑Basic Configuration]]) and re-include them into other servers.
This is needed especially for making them available to the WAN
([[#new_server_parts|🡑New Server Parts]]).
The LAN server becomes the //default_server// for all local addresses on port 80
via ''uci_listen_locally''.
It creates one of the following directives for every IP address of the LAN:
<code nginx>
	listen IPv4:80 default_server;
	listen [IPv6]:80 default_server;
</code>

If we want to make other servers also available on the LAN on port 80 through
their //server_name//, we can add a ''uci_listen_locally '80' '' to them by,
e.g.:

<code bash>uci add_list nginx.${EXAMPLE_COM//./_}.${LISTEN_LOCALLY}='80'</code>

This is possible for servers of the UCI config only.
For server parts in ''$(basename "${CONF_DIR}")/*.conf'' we can include
''${LAN_LISTEN}'' instead.
This file contains //listen// directives for all local IPs with fixed port 80
(and without //default_server//).

As the configuration depends on the local IPs, it is (re-)created when the LAN
interface changes.
Then it is validated and the Nginx service is reloaded.
We can do this also manually by:

<code bash> service nginx reload </code>


=== Additional Defaults for OpenWrt if Nginx is installed with SSL support ===

When Nginx is installed with SSL support, it will also create an automatically
managed ''$(basename "${LAN_SSL_LISTEN}")'' file in the directory
''$(dirname "${LAN_SSL_LISTEN}")/'' containing //listen// directives with fixed
port 443 and //ssl// parameter for all local IP addresses.

We can include this file into a //server {…}// part in a ''${CONF_DIR}*.conf''
file, for making the server available on the LAN via HTTPS.
For UCI servers we can use (similar as for HTTP):

<code nginx> list ${LISTEN_LOCALLY} '443 ssl' </code>

This is needed as the initial UCI config for SSL contains a //default_server//
for the LAN listening explicitly on local addresses, too.
There is also a server section that redirects requests for an inexistent
''server_name'' from HTTP to HTTPS. It acts as //default_server// if there is
++no other|; it uses an invalid name for that, more in the official
documentation on
[[https://nginx.org/en/docs/http/request_processing.html|request_processing]]
++:

$(code "/etc/config/nginx" "config-nginx-ssl")

When starting or reloading the Nginx service, the init.d looks which UCI servers
have set ''option ${MANAGE_SSL} 'self-signed' '', e.g., the LAN server.
For all those servers it checks if there is a certificate that is still valid
for 13 months or (re-)creates a self-signed one.
If there is any such server, it installs also a cron job that checks the
corresponding certificates once a year.
The option ''${MANAGE_SSL}'' is set to //'self-signed'// respectively removed
from a UCI server named ''${EXAMPLE_COM//./_}'' by the following
(see [[#ssl_server_parts|🡑SSL Server Parts]], too):

<code bash>
$(basename ${NGINX_UTIL}) ${ADD_SSL_FCT} ${EXAMPLE_COM//./_} \
# respectively: \
$(basename ${NGINX_UTIL}) del_ssl ${EXAMPLE_COM//./_}
</code>


EOF
