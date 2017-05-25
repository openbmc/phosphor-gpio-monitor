AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

sbin_PROGRAMS = \
	phosphor-gpio-util

phosphor_gpio_util_SOURCES = \
	gpio.cpp \
	main.cpp

phosphor_gpio_util_LDADD = \
	$(PHOSPHOR_LOGGING_LIBS)

phosphor_gpio_util_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS)
