project('miners', 'c',
  version : '0.1.0',
  default_options : [
    'warning_level=3',          # enables -Wall -Wextra
    'c_std=c23',
    'buildtype=debug',
  ]
)
# --- Debug-only flags ---
if get_option('buildtype') == 'debug'
  add_project_arguments('-fsanitize=address', language : 'c')
  add_project_link_arguments('-fsanitize=address', language : 'c')
endif
add_project_arguments('-D_POSIX_C_SOURCE=200809L', language : 'c')

# --- Dependencies ---
curses_dep = dependency('curses')    # Meson's built-in curses finder (prefers ncurses)
thread_dep = dependency('threads')   # portable pthreads
# --- Include directory ---
inc = include_directories('include')
# --- Executables ---
server_src = files('src/server/main.c','src/hashmap.c','src/server.c','src/protocol.c')
executable('server', server_src,
  include_directories : inc,
  dependencies : [thread_dep],
)
client_src = files('src/client/main.c')
executable('client', client_src,
  include_directories : inc,
  dependencies : [curses_dep, thread_dep],
)

test_proto = executable('test_proto',files('tests/test_protocol.c','src/protocol.c'),
  include_directories : inc,
  )
test('protocol',test_proto)

test_hm = executable('test_hm',
  files('tests/test_hashmap.c','src/hashmap.c'),
  include_directories : inc,
  )
test('hashmap',test_hm)

test_server = executable('test_server',
  files('tests/test_server.c','src/server.c','src/hashmap.c','src/protocol.c'),
  include_directories : inc,
  )
test('server',test_server)
