# meson.build
project('Wisp', 'c',
  version: '0.1',
  default_options: [
    'warning_level=3',
    'c_std=c23',
    'b_sanitize=address',
    'buildtype=debug',
  ]
)

sources = ['src/main.c',
           'src/readfile.c',
           'src/lexer.c',
           'src/parser.c',
           'src/vec.c',
           'src/arena.c']

add_project_arguments(['-Wall', '-Wextra', '-Wshadow'], language: 'c')

executable('Wisp',
  sources,
  install: true,
  c_args: ['-ggdb'],
)
