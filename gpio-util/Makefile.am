AM_DEFAULT_SOURCE_EXT = .cpp
AM_CPPFLAGS = -iquote ${top_srcdir}

sbin_PROGRAMS = \
	phosphor-gpio-util

phosphor_gpio_util_SOURCES = \
	argument.cpp \
	main.cpp

phosphor_gpio_util_LDADD = \
	$(PHOSPHOR_LOGGING_LIBS) \
	libphosphor_gpio.la

phosphor_gpio_util_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS)

libphosphor_gpiodir = ${libdir}
libphosphor_gpio_LTLIBRARIES = libphosphor_gpio.la
libphosphor_gpio_la_SOURCES = gpio.cpp
libphosphor_gpio_la_LDFLAGS = \
	$(PHOSPHOR_LOGGING_LIBS) -version-info 1:0:0 -shared
libphosphor_gpio_la_CXXFLAGS = $(PHOSPHOR_LOGGING_CFLAGS)

include_HEADERS = \
	gpio.hpp \
	../file.hpp
