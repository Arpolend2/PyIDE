import QtQuick 2.5
import Sailfish.Silica 1.0

Page {
    id: page
    objectName: "resultsPage"
    allowedOrientations: Orientation.All
    signal saveRequested()
    Shortcut {
        objectName: "resultsSaveShortcut"
        sequence: StandardKey.Save
        enabled: page.status === PageStatus.Active
        autoRepeat: false
        onActivated: page.saveRequested()
    }
    SilicaFlickable {
        anchors.fill: parent
        contentHeight: results.height + Theme.paddingLarge
        VerticalScrollDecorator {}
        PullDownMenu {
            MenuItem { text: qsTr("Stop"); enabled: runner.running; onClicked: runner.stop() }
            MenuItem { text: qsTr("Save output"); enabled: runner.output.length > 0; onClicked: pageStack.push(saveOutputDialog) }
        }
        Column {
            id: results
            width: parent.width
            spacing: Theme.paddingMedium
            PageHeader { title: qsTr("Results") }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: runner.status
                wrapMode: Text.Wrap
                color: Theme.highlightColor
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: runner.outputSaveError.length > 0
                text: runner.outputSaveError
                wrapMode: Text.Wrap
                color: Theme.errorColor
            }
            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Stop")
                visible: runner.running
                onClicked: runner.stop()
            }
            SectionHeader { text: qsTr("Output") }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: runner.outputTruncated
                text: qsTr("Output is limited to the last 1,048,576 characters; only this part can be saved.")
                color: Theme.secondaryColor
                wrapMode: Text.Wrap
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                text: runner.output || qsTr("No output")
                textFormat: Text.PlainText
                wrapMode: Text.WrapAnywhere
                font.family: "monospace"
            }
            SectionHeader { text: qsTr("Plots"); visible: runner.plots.length > 0 }
            Repeater {
                model: runner.plots
                delegate: Image {
                    width: results.width - 2 * Theme.horizontalPageMargin
                    x: Theme.horizontalPageMargin
                    height: sourceSize.width > 0 ? width * sourceSize.height / sourceSize.width : width
                    source: modelData
                    fillMode: Image.PreserveAspectFit
                    cache: false
                    asynchronous: true
                }
            }
            Component {
                id: saveOutputDialog
                Dialog {
                    canAccept: outputFileName.text.trim().length > 0
                    onAccepted: runner.saveOutput(outputFileName.text)
                    Column {
                        width: parent.width
                        DialogHeader { acceptText: qsTr("Save") }
                        TextField {
                            id: outputFileName
                            width: parent.width
                            label: qsTr("Output file name in Downloads")
                            placeholderText: "output.txt"
                        }
                    }
                }
            }
        }
    }
}
