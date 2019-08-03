project(
  'fdlinecombine', 'c',
  version: '0.5',
  license: 'MIT',
  default_options: 'buildtype=debugoptimized',
  meson_version: '>= 0.49.0',
)

project_name = meson.project_name()

i18n = import('i18n')

prefix = get_option('prefix')
datadir = join_paths(prefix, get_option('datadir'))

add_project_arguments('-DVERSION="@0@"'.format(meson.project_version()), language:'c')
add_project_arguments('-DGETTEXT_PACKAGE="@0@"'.format(project_name), language:'c')
add_project_arguments('-DLOCALE_DIR="@0@"'.format(join_paths(datadir, 'locale')), language:'c')

config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', project_name )

subdir('src')
subdir('po')
subdir('docs')
subdir('tests')
