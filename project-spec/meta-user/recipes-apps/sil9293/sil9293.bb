#
# This file is the sil9293 recipe.
#

SUMMARY = "Simple sil9293 application"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://sil9293.sh \
	"

INITSCRIPT_NAME = "sil9293"
INITSCRIPT_PARAMS = "defaults"

S = "${WORKDIR}"

inherit update-rc.d

do_install() {
	     install -d ${D}${sysconfdir}/init.d/
	     install -m 0755 sil9293.sh ${D}${sysconfdir}/init.d/sil9293
}
