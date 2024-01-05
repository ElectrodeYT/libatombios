project('libatombios', 'cpp',
    default_options: [
        'cpp_std=c++23'
    ]
)

libatombios_sources = [
    'src/atom.cpp',
    'src/bytecode.cpp',
    'src/command.cpp',
    'src/dumpToConsoles.cpp'
]

atombios_sources = [
    'src-test/main.cpp'
]

inc = include_directories('inc')

libatombios = shared_library('libatombios',
    libatombios_sources,
    include_directories : inc,
    install : true
)
libatombios_dep = declare_dependency(
    include_directories : inc,
    link_with : libatombios
)

executable('atombios',
    atombios_sources,
    dependencies : [ libatombios_dep ]
)
