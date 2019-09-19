test(
    'utest',
    executable(
        'utest', 'utest.cpp',
        dependencies: [
            libevdev,
            gmock,
            gtest,
            phosphor_dbus_interfaces,
            sdbusplus,
        ],
        implicit_include_directories: false,
        include_directories: '..',
        objects: [
            evdev_o,
            monitor_o,
        ]
    )
)
