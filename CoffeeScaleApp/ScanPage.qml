import QtQuick 2.12
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import Qt.labs.settings 1.0

Page {
    id: scanPage
	property bool quit: false
	property bool connected: deviceHandler.alive

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 30
        ///
        DeviceState {
            id: dstat
            height: childrenRect.height
        }

        ListView {
            id: listView
            width: parent.width
            Layout.fillHeight: true
            model: deviceFinder.devices
            clip: true
            Rectangle {
                width: parent.width
                height: parent.height
                x: 0
                z: -1
                border.color: "lightgrey"
                radius: 5
            }

            header: Rectangle {
                width: listView.width
                height: 50
                radius: 5
                z: 2
                color: "steelblue"
                Text {
                    anchors.centerIn: parent
                    text: qsTr("Found devices")
                    color: "white"
                    font.pixelSize: Qt.application.font.pixelSize * 1.6
                }
            }
            headerPositioning: ListView.OverlayHeader
            delegate: ItemDelegate {
                x: 5
                width: parent.width
                height: 80
                text: "<b>" + modelData.deviceName +
                      "</b> " + modelData.deviceAddress
                onClicked: {
                    deviceFinder.connectToService(modelData.deviceAddress)
                    settings.setValue("devName", modelData.deviceName)
                    settings.setValue("devAddr", modelData.deviceAddress)
                    dstat.update()
                }
            }
        }
        Button {
            id: scanButton
            text: qsTr("Scan")
            enabled: !deviceFinder.scanning
            onClicked: deviceFinder.startSearch()
            Layout.alignment: Qt.AlignRight
        }
    }
	footer: MyStatusBar { }
    Settings {
        id: settings
    }
	onConnectedChanged: {
		console.debug("onConnected")
		if (connected && quit)
		{
			onConnectTimer.start()
		}
	}
	Timer
	{
		id: onConnectTimer
		interval: 2000
		repeat: false
		running: false
		triggeredOnStart: false
		onTriggered: stackView.pop()
	}
	Connections {
		target: deviceFinder
		onConnectCalled: {
			console.debug("onConnectCalled")
			quit = true
		}
	}
}
