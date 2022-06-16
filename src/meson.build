boost_dep = dependency('boost',
                       version : '>=1.72',
                       modules : [ 'thread', 'filesystem', 'program_options', 'log', 'system' ])
openssl_dep = dependency('openssl')
thread_dep = dependency('threads')
cxx = meson.get_compiler('cpp')

src =  [ 'vis-config.cpp',
         'vis-session.cpp',
         'hvac-service.cpp',
         'hvac-can-helper.cpp',
         'hvac-led-helper.cpp',
         'main.cpp'
]
executable('agl-service-hvac',
           src,
           dependencies: [boost_dep, openssl_dep, thread_dep, systemd_dep],
           install: true,
           install_dir : get_option('sbindir'))
