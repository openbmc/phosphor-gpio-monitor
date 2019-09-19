project(
    'phosphor-gpio-monitor',
    'cpp',
    default_options: [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++17'
    ],
    license: 'Apache-2.0',
    version: '1.0',
)

build_tests = get_option('tests')

cppfs = meson.get_compiler('cpp').find_library('stdc++fs')
gmock = dependency('gmock', disabler: true, required: build_tests)
gtest = dependency('gtest', main: true, disabler: true, required: build_tests)
libevdev = dependency('libevdev')
libsystemd = dependency('libsystemd')
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
phosphor_logging = dependency('phosphor-logging')
sdbusplus = dependency('sdbusplus')

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

subdir('gpio-util')
subdir('presence')
subdir('test')
