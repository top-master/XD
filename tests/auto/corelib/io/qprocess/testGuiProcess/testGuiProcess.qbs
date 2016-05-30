import qbs

QProcessTestHelper {
    condition: Qt.widgets.present
    Depends { name: "Qt.widgets"; required: false }
}
