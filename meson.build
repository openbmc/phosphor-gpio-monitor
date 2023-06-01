project(
    'phosphor-gpio-monitor',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20',
        'buildtype=debugoptimized'
    ],
    license: 'Apache-2.0',
    version: '1.0',
    meson_version: '>=0.58.0',
)

cxx = meson.get_compiler('cpp')

build_tests = get_option('tests')

cppfs = meson.get_compiler('cpp').find_library('stdc++fs')
libevdev = dependency('libevdev')
libsystemd = dependency('libsystemd')
libgpiod = dependency('libgpiod')
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')
sdbusplus = dependency('sdbusplus')
systemd = dependency('systemd')

if cxx.has_header('nlohmann/json.hpp')
    nlohmann_json_dep = declare_dependency()
else
    subproject('nlohmann', required: false)
    nlohmann_json_dep = declare_dependency(
        include_directories: [
            'subprojects/nlohmann-json/single_include',
            'subprojects/nlohmann-json/single_include/nlohmann',
        ]
    )
endif

if cxx.has_header('CLI/CLI.hpp')
    cli11_dep = declare_dependency()
else
    cli11_dep = dependency('cli11')
endif

boost_args = ['-DBOOST_ASIO_DISABLE_THREADS',
              '-DBOOST_ERROR_CODE_HEADER_ONLY',
              '-DBOOST_SYSTEM_NO_DEPRECATED']

systemd_system_unit_dir = systemd.get_variable(
    'systemdsystemunitdir',
    pkgconfig_define: ['prefix', get_option('prefix')])

configure_file(
    copy: true,
    input: 'phosphor-gpio-monitor@.service',
    install: true,
    install_dir: systemd_system_unit_dir,
    output: 'phosphor-gpio-monitor@.service'
)

configure_file(
    copy: true,
    input: 'phosphor-multi-gpio-monitor.service',
    install: true,
    install_dir: systemd_system_unit_dir,
    output: 'phosphor-multi-gpio-monitor.service'
)

configure_file(
    copy: true,
    input: 'phosphor-gpio-presence@.service',
    install: true,
    install_dir: systemd_system_unit_dir,
    output: 'phosphor-gpio-presence@.service'
)

udev = dependency('udev')
udev_rules_dir = join_paths(
    udev.get_variable(
        'udevdir',
        pkgconfig_define: ['prefix', get_option('prefix')],
    ),
    'rules.d',
)

configure_file(
    copy: true,
    input: '99-gpio-keys.rules',
    install: true,
    install_dir: udev_rules_dir,
    output: '99-gpio-keys.rules'
)


configure_file(
    copy: true,
    input: 'phosphor-multi-gpio-monitor.json',
    install: true,
    install_dir: '/usr/share/phosphor-gpio-monitor',
    output: 'phosphor-multi-gpio-monitor.json'
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
    'argument.cpp',
    'mainapp.cpp',
    dependencies: [
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
   'gpioMon.cpp',
   dependencies: [
        cli11_dep,
        libgpiod,
        nlohmann_json_dep,
        phosphor_logging,
        sdbusplus,
    ],
    cpp_args: boost_args,
    install: true,
)

subdir('gpio-util')
subdir('presence')
subdir('test')
