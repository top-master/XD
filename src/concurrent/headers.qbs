import qbs

Group {
    name: "headers"
    files: [
        "qtconcurrent_global.h",
        "qtconcurrentcompilertest.h",
        "qtconcurrentexception.h",
        "qtconcurrentfilter.h",
        "qtconcurrentfilterkernel.h",
        "qtconcurrentfunctionwrappers.h",
        "qtconcurrentiteratekernel.h",
        "qtconcurrentmap.h",
        "qtconcurrentmapkernel.h",
        "qtconcurrentmedian.h",
        "qtconcurrentreducekernel.h",
        "qtconcurrentrun.h",
        "qtconcurrentrunbase.h",
        "qtconcurrentstoredfunctioncall.h",
        "qtconcurrentthreadengine.h",
    ]
    overrideTags: true
}
