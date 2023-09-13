import QtQuick 2.12
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import QtQuick.Window 2.12
import QtQml 2.0
import Qt.labs.settings 1.1

ApplicationWindow {
	id: window
	visible: true
	width: 480
	height: Screen.desktopAvailableHeight + 1
    title: qsTr("Scale")
	property date startTime: new Date()
	property bool devConnected: deviceHandler.alive
	property string addresses
	property var devices: []
	onClosing: {
		if (stackView.depth != 1) {
			close.accepted = false
			stackView.pop()
		} else {
			close.accepted = true
		}
	}
	header: ToolBar {
		contentHeight: toolButton.implicitHeight
		ToolButton {
			id: toolButton
			text: stackView.depth > 1 ? "\u25C0" : "\u2261"
			font.pixelSize: Qt.application.font.pixelSize * 1.6
			onClicked: {
				if (stackView.depth > 1) {
					stackView.pop()
				} else {
					drawer.open()
				}
			}
		}
		Label {
			text: stackView.currentItem.title ? stackView.currentItem.title : window.title
			anchors.centerIn: parent
		}
		ToolButton {
			text: "?"
			font.pixelSize: Qt.application.font.pixelSize * 1.6
			anchors.right: parent.right
			visible: stackView.depth <= 1
			enabled: stackView.depth <= 1
			onClicked: {
				if (stackView.depth <= 1)
					stackView.push("AboutPage.qml")
			}
		}
	}
	Drawer {
		id: drawer
		width: window.width * 0.66
		height: window.height
		Column {
			anchors.fill: parent
			Image {
				fillMode: Image.PreserveAspectFit
				width: parent.width * .75
				x: parent.width / 2 - width / 2
			}
			ItemDelegate {
				text: qsTr("Devices")
				width: parent.width
				onClicked: {
					stackView.push("ScanPage.qml")
					drawer.close()
				}
			}
			ItemDelegate {
				text: qsTr("Settings")
				width: parent.width
				onClicked: {
					stackView.push("SettingsPage.qml")
					drawer.close()
				}
			}
			ItemDelegate {
				text: qsTr("Quit")
				width: parent.width
				onClicked: {
					window.close()
				}
			}
		}
	}
	StackView {
		id: stackView
		anchors.fill: parent
		initialItem: "PlotPage.qml"
	}
	Dialog {
		id: dialog
		title: qsTr("Bluetooth required")
		standardButtons: Dialog.Cancel
		closePolicy: Popup.CloseOnEscape
		y: window.height / 3 - dialog.height / 3
		onRejected: window.close()
		visible: !connectionHandler.alive
		Text {
			id: name
			anchors.fill: parent
			anchors.margins: 10
			text: qsTr("This app requires Bluetooth in order to work, please turn Bluetooth on to continue.")
			wrapMode: Text.WordWrap
			width: dialog.width
		}
	}
	Settings {
		id: settings
	}
	function atemptAutoConnect(){
		devices = JSON.parse(settings.value("devices",
											'{"devices":[]}')).devices
		var devAddr = settings.value("devAddr", "")
		if (devAddr === "") {
			stackView.push("ScanPage.qml")
		} else if(connectionHandler.alive && !deviceHandler.alive) {
			deviceFinder.connectToService(devAddr)
		}
	}

	Component.onCompleted: atemptAutoConnect()
	onDevConnectedChanged: {

		if(deviceHandler.alive){
			delayedStartBLE.start()
		}
		deviceFinder.clearMessages()
		deviceHandler.clearMessages()
		var newDev = {
			"name": deviceHandler.deviceName(),
			"addr": deviceHandler.deviceAddr()
		}
		if (!devices.includes(newDev)) {
			//devices.push(deviceHandler.deviceAddr())
			devices.push(newDev)
			settings.setValue("devices", JSON.stringify({
															"devices": devices
														}))
		}
		console.log(JSON.stringify(devices))
	}
	Timer {
		id: delayedStartBLE
		running: false
		repeat: false
		triggeredOnStart: false
		interval: 2000
		onTriggered: {
			deviceHandler.start()
		}
	}
	Connections {
		target: Qt.application
		onStateChanged: {
			switch (Qt.application.state) {
				case Qt.ApplicationSuspended:
					deviceHandler.finalize()
					deviceHandler.disconnectService()
					console.log("suspended")
					break
				case Qt.ApplicationHidden:
					console.log("hidden")
					break
				case Qt.ApplicationActive:
					atemptAutoConnect()
					console.log("active")
					break
			}
		}
	}
}

/*##^##
Designer {
	D{i:1;anchors_height:100;anchors_width:100;anchors_x:180;anchors_y:261}
}
##^##*/

