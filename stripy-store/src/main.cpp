#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "backend.h"

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName("StripyOS");
    QGuiApplication::setApplicationName("stripy-store");
    QGuiApplication::setApplicationDisplayName("Stripy Store");
    QGuiApplication::setDesktopFileName("stripy-store");

    Backend backend;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("backend", &backend);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("Stripy", "Main");
    return app.exec();
}
