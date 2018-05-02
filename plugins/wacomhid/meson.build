cargs = ['-DG_LOG_DOMAIN="FuPluginWac"']

install_data(['wac-intuos.quirk'],
  install_dir: join_paths(datadir, 'fwupd', 'quirks.d')
)

shared_module('fu_plugin_wacomhid',
  sources : [
    'fu-wac-common.c',
    'fu-wac-device.c',
    'fu-wac-module.c',
    'fu-wac-module-bluetooth.c',
    'fu-wac-module-touch.c',
    'fu-plugin-wacomhid.c',
  ],
  include_directories : [
    include_directories('../..'),
    include_directories('../dfu'),
    include_directories('../../src'),
    include_directories('../../libfwupd'),
  ],
  install : true,
  install_dir: plugin_dir,
  c_args : cargs,
  dependencies : [
    plugin_deps,
  ],
  link_with : [
    dfu,
  ],
)

executable(
  'fu-wac-tool',
  sources : [
    'fu-wac-common.c',
    'fu-wac-device.c',
    'fu-wac-module.c',
    'fu-wac-module-bluetooth.c',
    'fu-wac-module-touch.c',
    'fu-wac-tool.c',
  ],
  include_directories : [
    include_directories('../..'),
    include_directories('../dfu'),
    include_directories('../../libfwupd'),
    include_directories('../../src'),
  ],
  dependencies : [
    plugin_deps,
  ],
  link_with : [
    dfu,
    fwupd,
    libfwupdprivate,
  ],
  c_args : cargs,
)
