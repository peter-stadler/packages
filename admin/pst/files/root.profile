#!/bin/sh

UPDATE_CMD="/bin/opkg update && /bin/opkg list-upgradable >/tmp/upgrades.lst"

[ -e /etc/crontabs/root ] && grep -q "${UPDATE_CMD}" /etc/crontabs/root || {
    echo "Installing cron job checking for upgradable packages ..."
    echo "54 03 * * * ${UPDATE_CMD}" >> /etc/crontabs/root &&
    /etc/init.d/cron reload
}

[ -e /tmp/upgrades.lst ] || {
    echo "Checking for upgradable packages ..."
    eval "${UPDATE_CMD}"
}

N_UPGRADABLE="$(cat /tmp/upgrades.lst | wc -l )"

[ "${N_UPGRADABLE}" -eq 0 ] && return 0

echo "Found outdated packages, update them by invoking:"
cat /tmp/upgrades.lst | cut -d" " -f1 | while read pkg
do echo "opkg upgrade '${pkg}' && sed -i '#${pkg}#d' /tmp/upgrades.lst && "
done

N_INSTALLED="$(opkg list-installed 2> /dev/null | wc -l)"
echo "echo '${N_UPGRADABLE} upgradable of ${N_INSTALLED} installed packages.'"

# echo "Upgrade ${N_UPGRADABLE} of ${N_INSTALLED} installed packages by invoking:"
# echo '  cat /tmp/upgrades.lst | cut -d" " -f1 | while read pkg'
# echo '  do opkg upgrade "$pkg" && sed -i "#$pkg#d" /tmp/upgrades.lst || break'
# echo '  done'

return 0
