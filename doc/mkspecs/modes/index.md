# Built-in modes and native names

Some behavior is not just a `.prf` convention: it is compiled into `qmake`
itself and changes how it resolves the spec, `CONFIG`, and the toolchain. This
folder documents those, plus the variables that sit right next to them in a
cross build.

- [host_build](./host_build.md): building the host tools (`moc`, `rcc`, `uic`,
  and the `qmake` bootstrap) natively during a cross build, so they run on the
  build machine while the rest of the tree targets another platform.
- [CROSS_COMPILE](./CROSS_COMPILE.md): the tool-name prefix that selects the
  cross toolchain, how the property, command-line, and environment channels
  differ, and why only the property reaches sub-projects.
- [native-checks](./native-checks.md): the scope names (`true`, `false`,
  `host_build`, the spec name) and variables (`TEMPLATE`, `QMAKE_PLATFORM`,
  `QMAKESPEC`, `REQUIRES`) that `qmake` recognizes in its own C++, before any
  feature file runs.
