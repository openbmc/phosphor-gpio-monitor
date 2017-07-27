AM_CPPFLAGS = -I$(top_srcdir)
AM_CXXFLAGS = $(LIBEVDEV_CFLAGS)

# Run all 'check' test programs
TESTS = $(check_PROGRAMS)

# # Build/add utest to test suite
check_PROGRAMS = utest
utest_CPPFLAGS = -Igtest $(GTEST_CPPFLAGS) $(AM_CPPFLAGS) $(PHOSPHOR_LOGGING_CFLAGS) $(LIBEVDEV_CFLAGS)
utest_CXXFLAGS = $(PTHREAD_CFLAGS)
utest_LDFLAGS = -lgtest_main -lgtest $(PTHREAD_LIBS) $(OESDK_TESTCASE_FLAGS) $(SYSTEMD_LIBS) $(PHOSPHOR_LOGGING_LIBS) $(LIBEVDEV_LIBS)
utest_SOURCES = utest.cpp
utest_LDADD = $(top_builddir)/monitor.o $(top_srcdir)/evdev.cpp
