fluidapp_sources = [
  'main.c',
  'fluidapp-window.c',
    'fluidapp-application.c',
    'libfluid.c',
  ]

cc = meson.get_compiler('c')

fluidapp_deps = [
  dependency('gtk4'),
  cc.find_library('m', required: true),
]

gnome = import('gnome')

fluidapp_sources += gnome.compile_resources('fluidapp-resources',
  'fluidapp.gresource.xml',
  c_name: 'fluidapp'
)

executable('fluidapp', fluidapp_sources,
  dependencies: fluidapp_deps,
  install: true,
)
