import QtQuick 2.12
import QtQuick.Controls 2.12

Rectangle {
	property bool errFlag:  deviceHandler.error != "" || deviceFinder.error != ""
	property bool hasMsg: errFlag || deviceFinder.info != ""
	width: parent.width
	height: 80
	color: errFlag ? "red" : "lime"
	visible: hasMsg
	MouseArea {
		anchors.fill: parent
		onClicked: {
			if (errFlag)
				stackView.push("ScanPage.qml")
		}
	}
	Label {
		id: name
		text: deviceHandler.error
			  != "" ? deviceHandler.error : deviceHandler.info
					  != "" ? deviceHandler.info : deviceFinder.error
							  != "" ? deviceFinder.error : deviceFinder.info
		anchors.centerIn: parent
		font.bold: true
		color: "white"
	}
}
