import QtQuick 2.5
import Sailfish.Silica 1.0
import Sailfish.Pickers 1.0
import Nemo.Notifications 1.0
import PythonRunner 1.0

Page {
    id: page
    objectName: "editorPage"
    allowedOrientations: Orientation.All
    property var resultsPage: null
    readonly property bool hasResults: runner.output.length > 0 || runner.plots.length > 0
    property bool showResultsWhenAvailable: false
    property bool searchVisible: false
    property string searchMessage: ""
    property string actionError: ""
    property bool editorReady: false
    property var suggestions: []
    property int completionStart: 0
    property var pendingOpenUrls: []
    forwardNavigation: !!resultsPage

    function attachResults() {
        if (pageStack.busy) {
            attachTimer.restart()
            return
        }
        if (!hasResults && resultsPage && pageStack.currentPage === resultsPage) {
            pageStack.navigateBack(PageStackAction.Immediate)
            attachTimer.restart()
        } else if (page.status === PageStatus.Active && pageStack.currentPage === page) {
            if (!hasResults && resultsPage) {
                resultsPage = null
                pageStack.popAttached(page, PageStackAction.Immediate)
            } else if (hasResults && !resultsPage) {
                resultsPage = pageStack.pushAttached(resultsComponent)
            }
            if (hasResults && resultsPage && showResultsWhenAvailable) {
                if (pageStack.busy) {
                    attachTimer.restart()
                } else {
                    showResultsWhenAvailable = false
                    pageStack.navigateForward()
                }
            }
        }
    }
    Connections {
        target: runner
        onOutputChanged: attachTimer.restart()
        onPlotsChanged: attachTimer.restart()
    }
    function requestOpen(url) {
        pendingOpenUrls = pendingOpenUrls.concat([url])
        openTimer.restart()
    }
    onStatusChanged: if (status === PageStatus.Active) {
        attachTimer.restart()
        if (pendingOpenUrls.length)
            openTimer.restart()
    }
    Component.onCompleted: {
        editor.text = editorDocument.text
        editorReady = true
        attachTimer.restart()
    }
    Timer { id: attachTimer; interval: 100; onTriggered: page.attachResults() }
    Timer {
        id: openTimer
        interval: 100
        onTriggered: {
            if (!page.pendingOpenUrls.length)
                return
            if (pageStack.busy) {
                restart()
                return
            }
            if (page.status !== PageStatus.Active) {
                if (pageStack.currentPage === page.resultsPage)
                    pageStack.navigateBack()
                return
            }
            var url = page.pendingOpenUrls[0]
            page.pendingOpenUrls = page.pendingOpenUrls.slice(1)
            if (editorDocument.dirty)
                pageStack.push(discardDialog, { action: "open", fileUrl: url })
            else
                editorDocument.openFile(url, false)
            if (!editorDocument.dirty && page.pendingOpenUrls.length)
                restart()
        }
    }
    Connections {
        target: fileOpenReceiver
        onRequested: page.requestOpen(url)
    }
    Component {
        id: resultsComponent
        ResultsPage {
            id: attachedResults
            onSaveRequested: page.saveFile()
            Component.onDestruction: if (page.resultsPage === attachedResults) page.resultsPage = null
        }
    }
    Shortcut {
        objectName: "saveShortcut"
        sequence: StandardKey.Save
        enabled: page.status === PageStatus.Active
        autoRepeat: false
        onActivated: page.saveFile()
    }
    Shortcut {
        objectName: "editorSearchShortcut"
        sequence: "Ctrl+F"
        enabled: page.status === PageStatus.Active
        onActivated: page.showSearch()
    }
    Shortcut {
        objectName: "searchCloseShortcut"
        sequence: "Escape"
        enabled: page.status === PageStatus.Active && page.searchVisible
                 && !searchInput.inputMethodComposing && !replaceInput.inputMethodComposing
                 && !editor.inputMethodComposing
        autoRepeat: false
        onActivated: page.searchVisible = false
    }
    Shortcut {
        objectName: "searchPreviousShortcut"
        sequence: "Up"
        enabled: page.status === PageStatus.Active && page.searchVisible && editor.enabled
                 && searchInput.text.length > 0
                 && (searchInput._editor.activeFocus || replaceInput._editor.activeFocus)
                 && !searchInput.inputMethodComposing && !replaceInput.inputMethodComposing
        onActivated: page.findText(true)
    }
    Shortcut {
        objectName: "searchNextShortcut"
        sequence: "Down"
        enabled: page.status === PageStatus.Active && page.searchVisible && editor.enabled
                 && searchInput.text.length > 0
                 && (searchInput._editor.activeFocus || replaceInput._editor.activeFocus)
                 && !searchInput.inputMethodComposing && !replaceInput.inputMethodComposing
        onActivated: page.findText(false)
    }
    Shortcut {
        objectName: "searchReplaceShortcut"
        sequence: "Tab"
        enabled: page.status === PageStatus.Active && page.searchVisible && editor.enabled
                 && searchInput.text.length > 0
                 && (searchInput._editor.activeFocus || replaceInput._editor.activeFocus)
                 && !searchInput.inputMethodComposing && !replaceInput.inputMethodComposing
                 && !editor.inputMethodComposing
        autoRepeat: false
        onActivated: page.replaceMatch()
    }
    function showSearch() {
        searchVisible = true
        searchInput.forceActiveFocus()
    }

    function updateCompletion() {
        var input = editor._editor
        suggestions = []
        if (!input.activeFocus || input.inputMethodComposing || input.selectionStart !== input.selectionEnd)
            return
        var position = input.cursorPosition
        var before = input.text.substring(0, position)
        var match = before.match(/[A-Za-z_][A-Za-z_0-9]*$/)
        if (!match || !highlighter.isCodePosition(position - 1))
            return
        completionStart = position - match[0].length
        var candidates = highlighter.completions(match[0])
        var atLineEnd = position === input.length || input.text.charAt(position) === "\n"
        suggestions = atLineEnd ? candidates : candidates.filter(function(candidate) {
            return /^[A-Za-z_][A-Za-z_0-9]*$/.test(candidate)
        })
    }
    function acceptCompletion(word) {
        var input = editor._editor
        if (!word || input.inputMethodComposing || input.selectionStart !== input.selectionEnd)
            return
        var position = input.cursorPosition
        if (editHelper.replace(completionStart, position, word)) {
            input.cursorPosition = completionStart + word.length - (/\(\)$/.test(word) ? 1 : 0)
            suggestions = []
        } else {
            actionError = qsTr("Could not insert the completion.")
        }
    }
    function findText(backwards) {
        var needle = searchInput.text
        if (!needle.length)
            return false
        var input = editor._editor
        var start = backwards ? input.selectionStart - 1 : input.selectionEnd
        var index = backwards ? (start < 0 ? -1 : input.text.lastIndexOf(needle, start))
                              : input.text.indexOf(needle, start)
        if (index < 0)
            index = backwards ? input.text.lastIndexOf(needle) : input.text.indexOf(needle)
        if (index < 0) {
            searchMessage = qsTr("No matches found.")
            return false
        }
        searchMessage = ""
        input.select(index, index + needle.length)
        var rect = input.positionToRectangle(index)
        var point = input.mapToItem(codePane.contentItem, rect.x, rect.y)
        codePane.contentY = Math.max(0, Math.min(codePane.contentHeight - codePane.height,
                                                point.y - codePane.height / 2))
        return true
    }
    function replaceMatch() {
        var input = editor._editor
        if (!searchInput.text.length || input.inputMethodComposing)
            return
        if (input.text.substring(input.selectionStart, input.selectionEnd) !== searchInput.text
                && !findText(false))
            return
        var start = input.selectionStart
        if (!editHelper.replace(start, input.selectionEnd, replaceInput.text)) {
            searchMessage = qsTr("Could not replace the match.")
            return
        }
        input.cursorPosition = start + replaceInput.text.length
        findText(false)
    }
    function replaceAllMatches() {
        var input = editor._editor
        if (!searchInput.text.length || input.inputMethodComposing)
            return
        var parts = input.text.split(searchInput.text)
        if (parts.length === 1) {
            searchMessage = qsTr("No matches found.")
            return
        }
        var position = input.cursorPosition
        if (!editHelper.replace(0, input.length, parts.join(replaceInput.text))) {
            searchMessage = qsTr("Could not replace the matches.")
            return
        }
        input.cursorPosition = Math.min(position, input.length)
        searchMessage = qsTr("Replaced %1 matches.").arg(parts.length - 1)
    }
    function openPicker() { pageStack.push(filePicker) }
    function newFile() {
        if (editorDocument.dirty)
            pageStack.push(discardDialog, { action: "new" })
        else
            editorDocument.newFile(false)
    }
    function saveFile() {
        if (editorDocument.filePath.length)
            editorDocument.save()
        else
            pageStack.push(saveDialog)
    }
    function runCode() {
        showResultsWhenAvailable = true
        runner.runEditor(editorDocument.text, editorDocument.filePath)
        attachTimer.restart()
    }

    SilicaFlickable {
        id: codePane
        anchors.fill: parent
        contentHeight: Math.max(height, editorContent.height + Theme.paddingLarge)
        VerticalScrollDecorator {}
        PullDownMenu {
            MenuLabel {
                objectName: "appInfoMenuLabel"
                text: "PyIDE " + Qt.application.version + "\n"
                      + qsTr("Python editor with NumPy and Matplotlib")
            }
            MenuItem { text: qsTr("Stop"); visible: runner.running; onClicked: runner.stop() }
            MenuItem { text: qsTr("Open"); onClicked: page.openPicker() }
            MenuItem { text: qsTr("New"); onClicked: page.newFile() }
            MenuItem { text: qsTr("Save"); onClicked: page.saveFile() }
            MenuItem {
                text: qsTr("Find and replace")
                onClicked: page.showSearch()
            }
            MenuItem { objectName: "runMenuItem"; text: qsTr("Run"); enabled: !runner.running; onClicked: page.runCode() }
        }
        Column {
            id: editorContent
            width: parent.width
            PageHeader { title: editorDocument.fileName + (editorDocument.dirty ? " *" : "") }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: editorDocument.error.length > 0 || editorDocument.cacheError.length > 0 || page.actionError.length > 0
                         || fileOpenReceiver.error.length > 0
                text: editorDocument.error || editorDocument.cacheError || page.actionError || fileOpenReceiver.error
                color: Theme.errorColor
                wrapMode: Text.Wrap
            }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * x
                visible: runner.status.length > 0 && !runner.finishedSuccessfully
                text: runner.status
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }
            SilicaFlickable {
                width: parent.width
                height: page.suggestions.length > 0 && editor._editor.activeFocus ? Theme.itemSizeSmall : 0
                contentWidth: completionRow.width + completionRow.x
                contentHeight: height
                flickableDirection: Flickable.HorizontalFlick
                clip: true
                Row {
                    id: completionRow
                    x: Theme.horizontalPageMargin
                    spacing: Theme.paddingSmall
                    Repeater {
                        model: page.suggestions
                        delegate: Button {
                            text: modelData
                            preferredWidth: Theme.buttonWidthExtraSmall
                            onClicked: page.acceptCompletion(modelData)
                        }
                    }
                }
                HorizontalScrollDecorator {}
            }
            TextArea {
                id: editor
                objectName: "pythonEditor"
                width: parent.width
                placeholderText: qsTr("Write Python code here")
                font.family: "monospace"
                color: Theme.colorScheme === Theme.LightOnDark ? "#dedede" : "#262626"
                onTextChanged: {
                    if (page.editorReady && text !== editorDocument.text)
                        editorDocument.text = text
                    completionTimer.restart()
                }
                Binding { target: editor._editor; property: "persistentSelection"; value: true }
                Connections {
                    target: editor._editor
                    onCursorPositionChanged: completionTimer.restart()
                    onActiveFocusChanged: completionTimer.restart()
                    onInputMethodComposingChanged: completionTimer.restart()
                }
                Text {
                    id: completionHint
                    objectName: "pythonCompletionHint"
                    parent: editor._editor
                    x: editor._editor.cursorRectangle.x
                    y: editor._editor.cursorRectangle.y
                    width: Math.min(implicitWidth, Math.max(0, parent.width - x))
                    clip: true
                    visible: editor._editor.activeFocus && page.suggestions.length > 0
                             && !editor._editor.inputMethodComposing
                             && (editor._editor.cursorPosition === editor._editor.length
                                 || editor._editor.text.charAt(editor._editor.cursorPosition) === "\n")
                    text: page.suggestions.length > 0
                          ? page.suggestions[0].substring(editor._editor.cursorPosition - page.completionStart) : ""
                    font: editor._editor.font
                    color: editor.color
                    opacity: 0.55
                    z: 1
                    MouseArea {
                        anchors.fill: parent
                        enabled: completionHint.visible
                        onClicked: page.acceptCompletion(page.suggestions[0])
                    }
                }
                PythonHighlighter {
                    id: highlighter
                    textDocument: editor._editor.textDocument
                    lightBackground: Theme.colorScheme !== Theme.LightOnDark
                }
                TextEditHelper {
                    id: editHelper
                    target: editor._editor
                    completionEnabled: page.suggestions.length > 0 && !editor._editor.inputMethodComposing
                    onCompletionAccepted: page.acceptCompletion(page.suggestions[0])
                }
            }
        }
    }

    Rectangle {
        id: searchPanel
        anchors.top: parent.top
        width: parent.width
        height: visible ? Math.min(searchContents.height, page.height * 0.7) : 0
        color: Theme.colorScheme === Theme.LightOnDark ? "black" : "white"
        border.color: Theme.secondaryColor
        visible: page.searchVisible
        z: 5
        SilicaFlickable {
            anchors.fill: parent
            contentHeight: searchContents.height
            clip: true
            Column {
                id: searchContents
                width: parent.width
                TextField {
                    id: searchInput
                    objectName: "searchInput"
                    width: parent.width
                    label: qsTr("Find")
                    placeholderText: qsTr("Find")
                    onTextChanged: page.searchMessage = ""
                    EnterKey.onClicked: page.findText(false)
                }
                TextField {
                    id: replaceInput
                    objectName: "replaceInput"
                    width: parent.width
                    label: qsTr("Replace with")
                    placeholderText: qsTr("Replace with")
                }
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    wrapMode: Text.Wrap
                    color: Theme.secondaryColor
                    text: page.searchMessage || qsTr("Search is case-sensitive and applies to the current file.")
                }
                SilicaFlickable {
                    id: searchActions
                    width: parent.width
                    height: Theme.itemSizeSmall
                    contentWidth: searchActionsRow.width
                    contentHeight: height
                    flickableDirection: Flickable.HorizontalFlick
                    clip: true
                    Row {
                        id: searchActionsRow
                        spacing: Theme.paddingSmall
                        readonly property real minimumWidth: searchPrevious.width + searchNext.width
                                                             + replaceButton.implicitWidth
                                                             + replaceAllButton.implicitWidth
                                                             + searchClose.width + 4 * spacing
                        width: Math.max(searchActions.width, minimumWidth)
                        height: searchActions.height
                        Button {
                            id: searchPrevious
                            objectName: "searchPrevious"
                            anchors.verticalCenter: parent.verticalCenter
                            preferredWidth: Theme.buttonWidthTiny
                            width: implicitHeight
                            icon.source: "image://theme/icon-m-up-alt"
                            Accessible.name: qsTr("Previous")
                            Accessible.role: Accessible.Button
                            enabled: editor.enabled && searchInput.text.length > 0
                            onClicked: page.findText(true)
                        }
                        Button {
                            id: searchNext
                            objectName: "searchNext"
                            anchors.verticalCenter: parent.verticalCenter
                            preferredWidth: Theme.buttonWidthTiny
                            width: implicitHeight
                            icon.source: "image://theme/icon-m-down-alt"
                            Accessible.name: qsTr("Next")
                            Accessible.role: Accessible.Button
                            enabled: editor.enabled && searchInput.text.length > 0
                            onClicked: page.findText(false)
                        }
                        Button {
                            id: replaceButton
                            objectName: "searchReplace"
                            anchors.verticalCenter: parent.verticalCenter
                            preferredWidth: Theme.buttonWidthExtraSmall
                            width: implicitWidth + (searchActionsRow.width - searchActionsRow.minimumWidth) / 2
                            text: qsTr("Replace")
                            enabled: editor.enabled && searchInput.text.length > 0
                            onClicked: page.replaceMatch()
                        }
                        Button {
                            id: replaceAllButton
                            objectName: "searchReplaceAll"
                            anchors.verticalCenter: parent.verticalCenter
                            preferredWidth: Theme.buttonWidthExtraSmall
                            width: implicitWidth + (searchActionsRow.width - searchActionsRow.minimumWidth) / 2
                            text: qsTr("Replace all")
                            enabled: editor.enabled && searchInput.text.length > 0
                            onClicked: page.replaceAllMatches()
                        }
                        Button {
                            id: searchClose
                            objectName: "searchClose"
                            anchors.verticalCenter: parent.verticalCenter
                            preferredWidth: Theme.buttonWidthTiny
                            width: implicitHeight
                            icon.source: "image://theme/icon-m-cancel"
                            Accessible.name: qsTr("Close")
                            Accessible.role: Accessible.Button
                            onClicked: page.searchVisible = false
                        }
                    }
                    HorizontalScrollDecorator {}
                }
            }
            VerticalScrollDecorator {}
        }
    }
    Timer { id: completionTimer; interval: 150; onTriggered: page.updateCompletion() }
    Connections {
        target: editorDocument
        onTextChanged: if (page.editorReady && editor.text !== editorDocument.text) editor.text = editorDocument.text
        onSaved: {
            saveNotification.previewBody = qsTr("Saved %1").arg(fileName)
            saveNotification.replacesId = 0
            saveNotification.publish()
        }
    }
    Connections {
        target: runner
        onOutputSaved: {
            saveNotification.previewBody = qsTr("Saved %1").arg(fileName)
            saveNotification.replacesId = 0
            saveNotification.publish()
        }
    }
    Notification {
        id: saveNotification
        objectName: "saveNotification"
        appName: "PyIDE"
        appIcon: "ru.arpolend.PyIDE"
        previewSummary: "PyIDE"
    }
    Component {
        id: filePicker
        FilePickerPage {
            title: qsTr("Select Python file")
            nameFilters: [ "*.py", "*.PY" ]
            onSelectedContentChanged: {
                if (selectedContent.toString().length)
                    page.requestOpen(selectedContent)
            }
        }
    }
    Component {
        id: discardDialog
        Dialog {
            objectName: "discardDialog"
            property string action
            property url fileUrl
            onAccepted: {
                if (action === "open")
                    editorDocument.openFile(fileUrl, true)
                else
                    editorDocument.newFile(true)
            }
            Column {
                width: parent.width
                DialogHeader { acceptText: qsTr("Discard") }
                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * x
                    text: qsTr("Discard unsaved changes?")
                    wrapMode: Text.Wrap
                }
            }
        }
    }
    Component {
        id: saveDialog
        Dialog {
            canAccept: fileNameInput.text.trim().length > 0
            onAccepted: editorDocument.saveNew(fileNameInput.text)
            Column {
                width: parent.width
                DialogHeader { acceptText: qsTr("Save") }
                TextField {
                    id: fileNameInput
                    width: parent.width
                    label: qsTr("File name in Downloads")
                    placeholderText: "script.py"
                }
            }
        }
    }
}
