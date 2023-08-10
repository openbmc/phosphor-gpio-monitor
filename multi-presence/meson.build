executable(
    'phosphor-multi-gpio-presence',
    'gpio_presence.cpp',
    'main.cpp',
    dependencies: [
        cli11_dep,
        libgpiod,
        nlohmann_json_dep,
        phosphor_logging,
        sdbusplus,
    ],
    cpp_args: boost_args,
    include_directories: '..',
    implicit_include_directories: false,
    install: true,
)