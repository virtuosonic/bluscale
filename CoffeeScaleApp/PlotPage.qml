import QtQuick 2.0
import QtQuick.Controls 2.3
import QtQuick.Layouts 1.3
import QmlPlotting 2.0 as QmlPlotting

Page {
    property variant samples: deviceHandler.samples
	property var cpArray: new ArrayBuffer(deviceHandler.maxSamples * 8 * 8)
	property var float64: new Float64Array(cpArray,0,deviceHandler.maxSamples * 8)
    property var mArr: []
    onSamplesChanged: {
        if (samples.length && deviceHandler.alive) {
            var currentSpl = samples[samples.length - 1]
            var txtSpl = currentSpl.toLocaleString(Qt.locale(),'f',1)
            txtSpl += qsTr(" g")
            scaleText.text = txtSpl
        }
        else
        {
            scaleText.text = "--"
        }
    }
    ColumnLayout {
        id: gridLayout
        anchors.fill: parent

        QmlPlotting.Axes {
            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.columnSpan: 3
			Layout.topMargin: 10
			Layout.leftMargin: 15
			Layout.rightMargin: 5
            //xLabel: "X Axis"
            //yLabel: "Y Axis"
            // Define plot group containing zoom/pan tool and XY plot item
            plotGroup: QmlPlotting.PlotGroup {
                viewRect: Qt.rect(0, -50 , 60, 600)
                plotItems: [

                    /*QmlPlotting.ZoomPanTool {
                                maxViewRect: Qt.rect(-2., -1., 4., 3.)
                                minimumSize: Qt.size(0.1, 0.1)
                            },*/
                    // XY plot item with default test data
                    QmlPlotting.XYPlot {
                        id: xyPlot
                        property alias ds: xyPlot.dataSource
                        dataSource: QmlPlotting.DataSource {}
						Component.onCompleted: dataSource.allocateData1D(deviceHandler.maxSamples)
                        //dataSource.setTestData1D()
                        lineColor: Qt.rgba(.5,.5,1,.8)
                        lineWidth: 8
                        //markerColor: Qt.hsva(hueSlider.hue, .7, .7, .6)
                        //markerSize: markerSizeSlider.value
                    }
                ]
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Text {
                id: scaleText
                Layout.margins: 20
                text:  "--"
                fontSizeMode: Text.FixedSize
                font.pixelSize: 36
            }
            Item {
                Layout.fillWidth: true
            }

            Text {
                id: chronoText
                height: 14
                Layout.margins: 20
                text: qsTr("00:00.000")
                font.pixelSize: 36
                fontSizeMode: Text.FixedSize
            }
        }
        RowLayout {
            Layout.fillWidth: true

            Button {
                id: zeroButton
                text: qsTr("Zero")
                Layout.margins: 20
                onClicked: deviceHandler.tare()
                enabled: deviceHandler.alive
            }
            Item {
                Layout.fillWidth: true
            }

            Button {
                id: button
                text: qsTr("Start")
                Layout.margins: 20
                onClicked: {
                    if (timer.running) {
                        button.text = qsTr("Start")
                        timer.stop()
                        chronoText.text = "00:00.000"
                    } else {
                        button.text = qsTr("Stop")
                        timer.start()
                        startTime = new Date()
                    }
                }
            }
        }
    }
    function plot()
    {
        try
        {
            samples.forEach(function(currentValue , index) {
                var indx = index*2
				float64[indx] = index *.1
                float64[indx+1] = currentValue
            })
			xyPlot.dataSource.copyFloat64Array1D(cpArray,samples.length * 2)
            xyPlot.update()
        }
        catch(e)
        {
            console.log("error writing to XYPlot: " + e)
        }
    }
    function fastPlot() {
        try
        {
            mArr.forEach(function(currentValue , index) {
                var indx = index*2
				float64[indx] = index * .25
                float64[indx+1] = currentValue
            })
			xyPlot.dataSource.copyFloat64Array1D(cpArray,mArr.length * 2)
            xyPlot.update()
        }
        catch(e)
        {
            console.log("error writing to XYPlot: " + e)
        }
    }
	footer: MyStatusBar{}

    Timer {
        id: oneSec
		interval: 250
        repeat: true
        running: true
        onTriggered: {
			if (samples)
            {
                mArr.push(samples[samples.length - 1])
				if(mArr.length > deviceHandler.maxSamples)
                {
                    mArr.splice(0,1)
                }
				fastPlot()
            }
        }
    }

    Timer {
        id: timer
        interval: 33
        running: false
        repeat: true
        onTriggered: {
            chronoText.text = ms2text(Math.abs(new Date - startTime))
        }
        function ms2text(millis) {
            var mnt = Math.floor(millis / 60000)
            millis %= 60000
            var sec = Math.floor(millis / 1000)
            millis %= 1000
            var timeText = "" + (mnt < 10 ? "0" + mnt : mnt) + ":"
                    + (sec < 10 ? "0" + sec : sec) + "."
                    + (millis < 100 ? "0" : "") + (millis < 10 ? "0" : "") + millis
            return timeText
        }
    }
}
