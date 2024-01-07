project('libatombios', 'cpp',
    default_options: [
        'cpp_std=c++23'
    ]
)

libatombios_sources = [
    'src/atom.cpp',
    'src/bytecode.cpp',
    'src/command.cpp',
    'src/dumpToConsoles.cpp',
    'src/iio.cpp'
]

atombios_sources = [
    'src-test/main.cpp'
]

inc = include_directories('inc')

build_atombios_tool = get_option('build_tools')

libatombios = static_library('atombios',
    libatombios_sources,
    include_directories : inc,
    install : true,
    pic : get_option('pic')
)
libatombios_dep = declare_dependency(
    include_directories : inc,
    link_with : libatombios
)

if build_atombios_tool
    executable('atombios',
        atombios_sources,
        dependencies : [ libatombios_dep ],
        install : true
    )
endif
