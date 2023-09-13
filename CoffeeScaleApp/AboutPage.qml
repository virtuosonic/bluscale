import QtQuick 2.0
import QtQuick.Controls 2.3

Page {
    id: aboutpage
    Text {
        id: name
        x: 196
        text: qsTr("Scale App")
        anchors.topMargin: parent.height / 3
        anchors.top: parent.top
        font.pointSize: 18
        anchors.horizontalCenter: parent.horizontalCenter
    }
    Text {
        id: version
        x: 301
        text: Qt.application.version
        font.pointSize: 12
        anchors.top: name.bottom
        anchors.topMargin: 30
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Text {
        id: element
        x: 309
        text: qsTr("Copyright © 2020,2023")
        anchors.top: version.bottom
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        font.pixelSize: 12
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}D{i:1;anchors_y:37}D{i:2;anchors_y:64}D{i:3;anchors_y:327}
}
##^##*/
