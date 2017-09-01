AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

sbin_PROGRAMS = \
	phosphor-gpio-presence

phosphor_gpio_presence_SOURCES = \
	main.cpp \
	argument.cpp \
	../evdev.cpp \
	gpio_presence.cpp

phosphor_gpio_presence_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(LIBEVDEV_CFLAGS)

phosphor_gpio_presence_LDADD = \
	-lstdc++fs \
	$(PHOSPHOR_LOGGING_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(LIBEVDEV_LIBS)

