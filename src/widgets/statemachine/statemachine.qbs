Group {
    prefix: path + "/"
    Group {
        condition: Qt.core.config.statemachine
        files: [
            "qguistatemachine.cpp",
        ]
    }
    Group {
        condition: Qt.core.config.qeventtransition
        files: [
            "qbasickeyeventtransition.cpp",
            "qbasickeyeventtransition_p.h",
            "qbasicmouseeventtransition.cpp",
            "qbasicmouseeventtransition_p.h",
            "qkeyeventtransition.cpp",
            "qkeyeventtransition.h",
            "qmouseeventtransition.cpp",
            "qmouseeventtransition.h",
        ]
    }
}
