import QtQuick 2.0
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.0
import Qt.labs.settings 1.0

Item {
    id: root
    property string name: settings.value("devName","")
    property string address: settings.value("devAddr","")
    property bool connected: deviceHandler.alive
	property bool connectable: !deviceHandler.alive && settings.value("devName","") !="" && settings.value("devAddr","") !=""

    function update() {
        name = settings.value("devName","")
        address = settings.value("devAddr","")
    }

    RowLayout {
        ColumnLayout {
            Layout.margins: 20
            Layout.fillWidth: true
            Text {
                text: qsTr("Current device")
                font.pixelSize: Qt.application.font.pixelSize * 1.6
            }
            Text {
                id: devname
                text: name != "" ? name : "----------"
                font.bold: true
                Layout.leftMargin: 20
            }
            Text {
                id: devAddr
                text: address != "" ? address : "0000"
                Layout.leftMargin: 20
            }
            Text {
                id: devState
                text: connected ? qsTr("connected") : qsTr("disconnected")
				color: connected ? "lime" : "red"
                Layout.leftMargin: 20
            }
        }
        Button {
            text: qsTr("Connect")
			onClicked: deviceFinder.connectToService(settings.value("devName",""),settings.value("devAddr",""))
			visible: connectable
			enabled: connectable
        }
    }


    Settings {
        id: settings
    }

}
