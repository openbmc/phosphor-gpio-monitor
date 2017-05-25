sbin_PROGRAMS = phosphor-gpio-monitor

noinst_HEADERS = monitor.hpp

phosphor_gpio_monitor_SOURCES = \
				argument.cpp \
				monitor.cpp \
				mainapp.cpp

phosphor_gpio_monitor_LDFLAGS = $(SYSTEMD_LIBS) \
                                $(SDBUSPLUS_LIBS) \
                                $(PHOSPHOR_LOGGING_LIBS) \
                                $(LIBEVDEV_LIBS)

phosphor_gpio_monitor_CXXFLAGS = $(SYSTEMD_CFLAGS) \
                                 $(SDBUSPLUS_CFLAGS) \
                                 $(PHOSPHOR_LOGGING_CFLAGS) \
                                 $(LIBEVDEV_CFLAGS)

SUBDIRS = test gpio-util
