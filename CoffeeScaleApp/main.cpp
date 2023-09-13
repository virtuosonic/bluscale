#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QLoggingCategory>
#include <QQmlContext>


#include "connectionhandler.h"
#include "devicefinder.h"
#include "devicehandler.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth* = true"));

	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth.windows = true"));
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth.winrt = true"));
	QLoggingCategory::setFilterRules(QStringLiteral("qt.bluetooth.qml = true"));
    QGuiApplication app(argc, argv);
	#ifndef __ANDROID__
	app.setApplicationVersion("0.0.4");
	#endif

    ConnectionHandler connectionHandler;
    DeviceHandler deviceHandler;
    DeviceFinder deviceFinder(&deviceHandler);

    qmlRegisterUncreatableType<DeviceHandler>("Shared", 1, 0, "AddressType", "Enum is not a type");

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("connectionHandler", &connectionHandler);
    engine.rootContext()->setContextProperty("deviceFinder", &deviceFinder);
    engine.rootContext()->setContextProperty("deviceHandler", &deviceHandler);
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
