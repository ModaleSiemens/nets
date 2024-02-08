project(
    'Modale\'s "nets"',
    'cpp',
    version: '1.0.0',

    default_options: [
        'cpp_std=c++23'
    ]
)

subdir('src')

sockets_dep = [] 

# Platform specific sockets library
if host_machine.system() == 'windows'
    sockets_dep += meson.get_compiler('cpp').find_library(
        'ws2_32'
    )
endif

deps = [
    sockets_dep
]

inc = include_directories('include')

lib_nets = library(
    'nets',
    src,

    include_directories: inc,
    dependencies       : deps
)

lib_nets_dep = declare_dependency(
    include_directories: inc,
    link_with          : lib_nets,
    dependencies       : deps
)

subdir('tests')