project(
    'phosphor-gpio-monitor',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++23',
        'buildtype=debugoptimized'
    ],
    license: 'Apache-2.0',
    version: '1.0',
    meson_version: '>=1.1.1',
)

cxx = meson.get_compiler('cpp')

libevdev = dependency('libevdev')
libsystemd = dependency('libsystemd')
libgpiod = dependency('libgpiod')
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')
sdbusplus = dependency('sdbusplus')
systemd = dependency('systemd')

nlohmann_json_dep = dependency('nlohmann_json', include_type: 'system')

if cxx.has_header('CLI/CLI.hpp')
    cli11_dep = declare_dependency()
else
    cli11_dep = dependency('CLI11')
endif

boost_args = ['-DBOOST_ASIO_DISABLE_THREADS',
              '-DBOOST_ERROR_CODE_HEADER_ONLY',
              '-DBOOST_SYSTEM_NO_DEPRECATED']

systemd_system_unit_dir = systemd.get_variable(
    'systemdsystemunitdir',
    pkgconfig_define: ['prefix', get_option('prefix')])

fs = import('fs')
fs.copyfile(
    'phosphor-gpio-monitor@.service',
    install: true,
    install_dir: systemd_system_unit_dir
)

fs.copyfile(
    'phosphor-multi-gpio-monitor.service',
    install: true,
    install_dir: systemd_system_unit_dir
)

fs.copyfile(
    'phosphor-multi-gpio-presence.service',
    install: true,
    install_dir: systemd_system_unit_dir
)

fs.copyfile(
    'phosphor-gpio-presence@.service',
    install: true,
    install_dir: systemd_system_unit_dir
)

udev = dependency('udev')
udev_rules_dir = join_paths(
    udev.get_variable(
        'udevdir',
        pkgconfig_define: ['prefix', get_option('prefix')],
    ),
    'rules.d',
)

fs.copyfile(
    '99-gpio-keys.rules',
    install: true,
    install_dir: udev_rules_dir
)

fs.copyfile(
    'phosphor-multi-gpio-monitor.json',
    install: true,
    install_dir: get_option('datadir') / 'phosphor-gpio-monitor'
)

libevdev_o = static_library(
    'libevdev_o',
    'evdev.cpp',
    dependencies: [
        libevdev,
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
    ]
)

libmonitor_o = static_library(
    'libmonitor_o',
     'monitor.cpp',
    dependencies: [
        libevdev,
        libsystemd,
        phosphor_logging,
    ],
    link_with: [
        libevdev_o,
    ],
)

phosphor_gpio_monitor = executable(
    'phosphor-gpio-monitor',
    'mainapp.cpp',
    dependencies: [
        cli11_dep,
        libevdev,
        libsystemd,
        phosphor_logging,
    ],
    install: true,
    link_with: [
        libevdev_o,
        libmonitor_o,
    ],
)

executable(
   'phosphor-multi-gpio-monitor',
   'gpioMonMain.cpp',
   'gpioEdgeMon.cpp',
   'pulseMon.cpp',
   'inventoryAction.cpp',
   'systemdAction.cpp',
   dependencies: [
        cli11_dep,
        libgpiod,
        nlohmann_json_dep,
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
    ],
    cpp_args: boost_args,
    install: true,
)

executable(
   'phosphor-multi-gpio-presence',
   'gpioMonMain.cpp',
   'gpioEdgeMon.cpp',
   'pulseMon.cpp',
   'inventoryAction.cpp',
   'systemdAction.cpp',
   dependencies: [
        cli11_dep,
        libgpiod,
        nlohmann_json_dep,
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
    ],
    cpp_args: boost_args,
    install: true,
)

subdir('presence')

build_tests = get_option('tests')
if build_tests.allowed()
    subdir('test')
endif
