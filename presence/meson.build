executable(
    'phosphor-gpio-presence',
    'main.cpp',
    'gpio_presence.cpp',
    dependencies: [
        libevdev,
        phosphor_logging,
    ],
    include_directories: '..',
    implicit_include_directories: false,
    install: true,
    link_with: [
        libevdev_o,
    ],
)
