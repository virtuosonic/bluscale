import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.1

Page {
    id: settpage
    title: qsTr("Scale Settings")
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        DeviceState {
            id: devstat
            height: childrenRect.height
        }

        RowLayout {
            spacing: 20
            Layout.alignment: Qt.AlignHCenter | Qt.AlignLeft
            Layout.fillWidth: false
            Text {
                text: qsTr("Reference")
            }
            TextField {
                id: calibrationReference
                validator: DoubleValidator {
                    bottom: 0
                    top: 500
                    decimals: 2
                }

                placeholderText: "100"
                font.pixelSize: 12
            }
        }
        RowLayout {
            Button {
                id: buttonCal
                text: qsTr("Calibrate")
            }
            Button {
                id: buttonZero
                text: qsTr("Zero")
            }
        }

        Rectangle {
            width: parent.width
            height: childrenRect.height
            z: -1
            border.color: "lightgrey"
            radius: 5
            ButtonGroup {
                id: buttonGroup
            }
            ListView {
                id: radiobox1
				height: childrenRect.height
                interactive: false
                Layout.fillWidth: true
                model: [qsTr("Grams"), qsTr("Ounces")]
                delegate: RadioDelegate {
                    text: modelData
                    checked: index === settings.mode
                    ButtonGroup.group: buttonGroup
                    onClicked: radiobox1.currentIndex = index
					padding: 20
                }
				header: Label {
                    text: qsTr("Mode:")
					width: parent.width
					padding:20
					font.bold: true
                }
            }
        }
    }
    Settings {
        id: settings
        property alias mode: radiobox1.currentIndex
        property alias reference: calibrationReference.text
    }
}
/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/

