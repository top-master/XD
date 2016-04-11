import qbs
import qbs.FileInfo

QtProduct {
    type: ["application"]
    property bool install: true
    property string installDir: FileInfo.relativePath(project.qtbasePrefix, sourceDirectory)

    Depends { name: "examples (deployment)"; profiles: [project.hostProfile] }
    Depends { name: "Qt.core" }

    cpp.defines: []

    Group {
        fileTagsFilter: type
        qbs.install: install
        qbs.installDir: installDir
    }
}
