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

executable(
    'phosphor-gpio-monitor',
    'argument.cpp',
    'evdev.cpp',
    'mainapp.cpp',
    'monitor.cpp',
    dependencies: [
        cppfs,
        libevdev,
        libsystemd,
        phosphor_dbus_interfaces,
        phosphor_logging,
        sdbusplus,
    ],
    install: true,
)

#subdir('gpio-util')
#subdir('presence')
#subdir('test')
