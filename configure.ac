# Initialization
AC_PREREQ([2.69])
AC_INIT([phosphor-gpio-monitor], [1.0], [https://github.com/openbmc/phosphor-gpio-monitor/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_17([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

# For linking
LT_INIT

# Checks for modules
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221])
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging])
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus])
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces])
PKG_CHECK_MODULES([LIBEVDEV], [libevdev])
PKG_CHECK_MODULES([LIBGPIOD], [libgpiod])

# Download gpio.h from github if necessary.
AC_CHECK_HEADER(linux/gpio.h,[HAVE_LINUX_GPIO_H=""],[HAVE_LINUX_GPIO_H="-I linux/gpio.h"])
AS_IF([test "$HAVE_LINUX_GPIO_H" != ""],
    AC_MSG_WARN([Could not find linux/gpio.h: Attempting to download locally for building from https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/gpio.h])
    AC_SUBST([BT_BMC_DL],[`mkdir -p linux;wget https://raw.githubusercontent.com/torvalds/linux/master/include/uapi/linux/gpio.h -O linux/gpio.h`])
)

# Check for json header
AC_CHECK_HEADER(
    nlohmann/json.hpp,
    [],
    [AC_MSG_ERROR([Could not find nlohmann/json.hpp])]
)

# Check for CLI header
AC_CHECK_HEADER(
    CLI/CLI.hpp,
    [],
    [AC_MSG_ERROR([Could not find CLI/CLI.hpp])]
)

# check for boost headers
AC_CHECK_HEADER(
    boost/asio/posix/stream_descriptor.hpp,
    [],
    [AC_MSG_ERROR([Could not find boost/asio/posix/stream_descriptor.hpp])]
)
AX_APPEND_COMPILE_FLAGS(["-DBOOST_ASIO_DISABLE_THREADS"], [CPPFLAGS])
AX_APPEND_COMPILE_FLAGS(["-DBOOST_ERROR_CODE_HEADER_ONLY"], [CPPFLAGS])
AX_APPEND_COMPILE_FLAGS(["-DBOOST_SYSTEM_NO_DEPRECATED"], [CPPFLAGS])

# Check/set gtest specific functions.
AX_PTHREAD([GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"],[GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=0"])
AC_SUBST(GTEST_CPPFLAGS)
AC_ARG_ENABLE([oe-sdk],
    AS_HELP_STRING([--enable-oe-sdk], [Link testcases absolutely against OE SDK so they can be ran within it.])
)
AC_ARG_VAR(OECORE_TARGET_SYSROOT,
    [Path to the OE SDK SYSROOT])
AS_IF([test "x$enable_oe_sdk" == "xyes"],
    AS_IF([test "x$OECORE_TARGET_SYSROOT" == "x"],
          AC_MSG_ERROR([OECORE_TARGET_SYSROOT must be set with --enable-oe-sdk])
    )
    AC_MSG_NOTICE([Enabling OE-SDK at $OECORE_TARGET_SYSROOT])
    [
        testcase_flags="-Wl,-rpath,\${OECORE_TARGET_SYSROOT}/lib"
        testcase_flags="${testcase_flags} -Wl,-rpath,\${OECORE_TARGET_SYSROOT}/usr/lib"
        testcase_flags="${testcase_flags} -Wl,-dynamic-linker,`find \${OECORE_TARGET_SYSROOT}/lib/ld-*.so | sort -r -n | head -n1`"
    ]
    AC_SUBST([OESDK_TESTCASE_FLAGS], [$testcase_flags])
)

# Create configured output
AC_CONFIG_FILES([Makefile gpio-util/Makefile presence/Makefile test/Makefile])
AC_OUTPUT
