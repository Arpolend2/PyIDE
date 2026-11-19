import QtQuick 2.0
import Sailfish.Silica 1.0

ApplicationWindow {
    id: appWindow
    property bool coverShowsResults: false
    allowedOrientations: Orientation.All
    initialPage: Component { EditorPage {} }
    Connections {
        target: appWindow.pageStack
        onCurrentPageChanged: {
            var current = appWindow.pageStack.currentPage
            if (current && current.objectName === "editorPage")
                appWindow.coverShowsResults = false
            else if (current && current.objectName === "resultsPage")
                appWindow.coverShowsResults = true
        }
    }
    cover: Component {
        CoverBackground {
            objectName: "appCover"
            Label {
                id: coverTitle
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.paddingMedium
                text: appWindow.coverShowsResults ? qsTranslate("ResultsPage", "Results") : editorDocument.fileName
                font.pixelSize: Theme.fontSizeExtraSmall
                elide: Text.ElideRight
            }
            Label {
                objectName: "coverText"
                anchors.top: coverTitle.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.paddingMedium
                visible: !appWindow.coverShowsResults || runner.output.length > 0 || runner.plots.length === 0
                text: appWindow.coverShowsResults ? (runner.output || qsTranslate("ResultsPage", "No output"))
                                                   : editorDocument.text
                textFormat: Text.PlainText
                font.family: "monospace"
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
                clip: true
            }
            Image {
                anchors.top: coverTitle.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: Theme.paddingMedium
                visible: appWindow.coverShowsResults && runner.output.length === 0 && runner.plots.length > 0
                source: visible ? runner.plots[runner.plots.length - 1] : ""
                fillMode: Image.PreserveAspectFit
                cache: false
                asynchronous: true
            }
        }
    }
}
