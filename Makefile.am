bin_PROGRAMS = phosphor-gpio-monitor gpioMonitor

noinst_HEADERS = monitor.hpp gpioMon.hpp

phosphor_gpio_monitor_SOURCES = \
	argument.cpp \
	monitor.cpp \
	evdev.cpp \
	mainapp.cpp

phosphor_gpio_monitor_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(LIBEVDEV_LIBS)

phosphor_gpio_monitor_CXXFLAGS = \
	$(SYSTEMD_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(LIBEVDEV_CFLAGS)

gpioMonitor_SOURCES = \
	argument.cpp \
	gpioMon.cpp \
	gpioMonMain.cpp

gpioMonitor_LDFLAGS = \
	$(SYSTEMD_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(LIBGPIOD_LIBS)

gpioMonitor_CXXFLAGS = \
	$(SYSTEMD_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(LIBGPIOD_CFLAGS)

SUBDIRS = test gpio-util presence
