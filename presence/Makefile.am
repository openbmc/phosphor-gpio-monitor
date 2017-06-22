AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

sbin_PROGRAMS = \
	phosphor-gpio-presence

phosphor_gpio_presence_SOURCES = \
	main.cpp \
	argument.cpp \
	gpio_presence.cpp

phosphor_gpio_presence_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS)

phosphor_gpio_presence_LDADD = \
	$(PHOSPHOR_LOGGING_LIBS)

