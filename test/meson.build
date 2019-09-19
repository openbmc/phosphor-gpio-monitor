test(
    'utest',
    executable(
        'utest', 'utest.cpp',
        dependencies: [
            gmock,
            gtest,
            libevdev,
        ],
        implicit_include_directories: false,
        include_directories: '..',
        link_with: [
            libevdev_o,
            libmonitor_o,
        ]
    )
)
