#
# This file is the serd recipe.
#

SUMMARY = "Update /etc/inetd.conf for serd"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://serd.inetd.conf \
	"

S = "${WORKDIR}"


do_install() {
	install -d ${D}${sysconfdir}
	install -m 0644 serd.inetd.conf ${D}${sysconfdir}/inetd.conf
}
