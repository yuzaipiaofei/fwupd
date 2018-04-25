cargs = ['-DG_LOG_DOMAIN="FuPluginQmi"']

install_data([
  'qmi-gobi3000.quirk',
  ],
  install_dir: join_paths(datadir, 'fwupd', 'quirks.d')
)

shared_module('fu_plugin_qmi',
  sources : [
    'fu-qmi-device.c',
    'fu-plugin-qmi.c',
  ],
  include_directories : [
    include_directories('../..'),
    include_directories('../../src'),
    include_directories('../../libfwupd'),
  ],
  install : true,
  install_dir: plugin_dir,
  c_args : cargs,
  dependencies : [
    plugin_deps,
    libqmi,
  ],
)
