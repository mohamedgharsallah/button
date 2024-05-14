
DESCRIPTION = "GPIO Reading Service"
LICENSE = "CLOSED"

SRC_URI = "file://gpio_service.cpp \
           file://gpio.service"

DEPENDS += "sqlite3 "

S = "${WORKDIR}"



do_compile() {
    ${CXX} ${CXXFLAGS} ${LDFLAGS} -o gpio_service gpio_service.cpp -lsqlite3
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 gpio_service ${D}${bindir}

    install -d ${D}${systemd_unitdir}/system
    install -m 0644 ${WORKDIR}/gpio.service ${D}${systemd_unitdir}/system
}

FILES_${PN} += "${systemd_unitdir}/system/gpio.service"

SYSTEMD_SERVICE_${PN} = "gpio.service"

inherit systemd

