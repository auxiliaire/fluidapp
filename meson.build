project('fluidapp', 'c',
          version: '0.1.0',
    meson_version: '>= 0.59.4',
  default_options: [ 'warning_level=3',
                     'c_std=gnu2x',
                     'optimization=3',
                   ],
)

i18n = import('i18n')

config_h = configuration_data()
config_h.set_quoted('PACKAGE_VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', 'fluidapp')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
configure_file(
  output: 'fluidapp-config.h',
  configuration: config_h,
)
add_project_arguments([
  '-I' + meson.project_build_root(),
  '-W',
  '-Wpedantic',
  '-ffast-math',
  '-march=native',
], language: 'c')


subdir('data')
subdir('src')
subdir('po')

meson.add_install_script('build-aux/meson/postinstall.py')
