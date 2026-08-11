# mkspecs

XD's qmake mkspecs: the machine-and-compiler spec files plus the feature
(`.prf`) files under `mkspecs/`. This page is the index.

- [Load order](./load-order.md): the order qmake loads the feature files around
  a project, and which of them have the full `CONFIG` settled yet. Read this
  before adding a config decision (anything that reads or edits `CONFIG` or
  `QT_CONFIG`), so it lands where the values it needs are already set.
- [Built-in modes](./modes/index.md): build modes baked into `qmake` itself, not
  just `.prf` conventions. Starts with host builds.
- [Linking and `.prl`](./linking.md): how a library's transitive dependencies
  reach its consumers through `.prl` files, the `staticlib`-vs-`explicitlib` rule
  (and why `explicitlib` is unix-only yet static libs still propagate on
  Windows), and the stale-prl regeneration trap that breaks static-lib links.
