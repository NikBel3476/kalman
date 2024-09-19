import QtQuick 2.15
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    SystemPalette { id: myPalette; colorGroup: SystemPalette.Active }
    color: myPalette.window
    anchors.fill: parent

    ColumnLayout {
        spacing: 10
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        focusPolicy: Qt.TabFocus

        TextField {
            placeholderText: qsTr("Username")
        }
        TextField {
            echoMode: TextInput.Password
            placeholderText: qsTr("Password")
        }
    }
}
