gmock_dep = dependency('gmock', disabler: true, required: build_tests)
gtest_dep = dependency('gtest', main: true, disabler: true, required: build_tests)
if not gtest_dep.found() or not gmock_dep.found()
    gtest_proj = import('cmake').subproject('googletest', required: false)
    if gtest_proj.found()
        gtest_dep = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ]
        )
        gmock_dep = gtest_proj.dependency('gmock')
    else
        assert(
            not get_option('tests').allowed(),
            'Googletest is required if tests are enabled'
        )
    endif
endif

test(
    'utest',
    executable(
        'utest', 'utest.cpp',
        dependencies: [
            gmock_dep,
            gtest_dep,
            libevdev,
            sdbusplus,
        ],
        implicit_include_directories: false,
        include_directories: '..',
        link_with: [
            libevdev_o,
            libmonitor_o,
        ]
    )
)
