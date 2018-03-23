import qbs

Group {
    name: "statemachine"
    condition: config.statemachine
    prefix: path + '/'
    files: [
        "qabstractstate.cpp",
        "qabstractstate.h",
        "qabstractstate_p.h",
        "qabstracttransition.cpp",
        "qabstracttransition.h",
        "qabstracttransition_p.h",
        "qfinalstate.cpp",
        "qfinalstate.h",
        "qfinalstate_p.h",
        "qhistorystate.cpp",
        "qhistorystate.h",
        "qhistorystate_p.h",
        "qsignaleventgenerator_p.h",
        "qsignaltransition.cpp",
        "qsignaltransition.h",
        "qsignaltransition_p.h",
        "qstate.cpp",
        "qstate.h",
        "qstate_p.h",
        "qstatemachine.cpp",
        "qstatemachine.h",
        "qstatemachine_p.h",
    ]

    Group {
        name: "qeventtransition"
        condition: product.config.qeventtransition
        files: [
            "qeventtransition.cpp",
            "qeventtransition.h",
            "qeventtransition_p.h",
        ]
    }
}
