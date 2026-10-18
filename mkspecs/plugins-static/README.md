# Static plugin descriptors

Each `<name>.pri` here describes one static plugin of `src/plugins-static`: its
include folder, headers, sources, Qt modules, and the static plugins it needs.
The plugin's own project (`src/plugins-static/<name>/<name>.pro`) writes it when
qmake runs on that project; hence a build of `src/plugins-static` comes before
the projects that use a plugin. Git ignores every file here but this one.

A project uses a static plugin by name:

```
STATIC_PLUGINS += ignore-parser
```

A project that uses Qt may write `QTPLUGIN_STATIC` instead, an alias that
counts only while `CONFIG` holds `qt`. A plugin that needs Qt modules stops
qmake with an error in a project without `qt`.

Every project has `static_plugins` in its `CONFIG` by default; a project opts
out of static plugins altogether with `CONFIG -= static_plugins`.

which compiles the plugin's sources into the project, adds its folder to the
include path and its Qt modules to `QT`, after the static plugins it needs. A
plugin that is not built yet stops qmake with an error that names it.
