
If the `qt_static` setting is enabled by your `.pro` file, like:
```
CONFIG += qt_static
```
Then that should enable `packing` setting for release builds.

And whenever `packing` is enabled, once the build finishes the packer script that
resides in this folder gets called, and said script redirects to proper UPX executalbe.

If above behavior is undesired, then try:
```
CONFIG += qt_static no_packing
```

## Security

Unfortunately, some anti-virus programs show UPX's original `.exe` files as virus/trojan files.
See [virustotal.com](https://virustotal.com)

**However,** You can compare our `.exe` binary files with the binaries of
```
UPX version 5.1.0
```
And see for yourself that these are byte-per-byte same as UPX's GitHub release.
