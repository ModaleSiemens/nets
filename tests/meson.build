tests = [
    [
        'Test1',
        'test1.cpp'
    ],
    [
        'ServerTest',
        'tcp_server.cpp'
    ],
    [
        'ClientTest',
        'tcp_client.cpp'
    ]
]

foreach test : tests
    executable(
        test[0],
        test[1],

        dependencies: lib_nets_dep,

        link_args: 
        [
            '-lstdc++exp' # Enable std::print, std::println
        ]
    )
endforeach