#include <QApplication>
#include <QSurfaceFormat>
#include <QMessageBox>

#include "mainwindow.h"
#include "splashscreen.h"

#define GL_MAJOR 4
#define GL_MINOR 0

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Create splash screen.
    SplashScreen splashScreen;
    splashScreen.show();
    app.processEvents();

    // Customize GUI style elements.
    app.setStyleSheet("QGroupBox { font-weight: bold; }");

    // Setup default context attributes.
    QSurfaceFormat surfaceFormat;
    surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
    surfaceFormat.setDepthBufferSize(24);
    surfaceFormat.setStencilBufferSize(8);
    surfaceFormat.setRenderableType(QSurfaceFormat::OpenGL);
    surfaceFormat.setVersion(GL_MAJOR, GL_MINOR);
    QSurfaceFormat::setDefaultFormat(surfaceFormat);

    // Create main application window.
    MainWindow window;
    QObject::connect(&window,       SIGNAL(initialisationProgress(int, const QString&)),
                     &splashScreen, SLOT(updateProgress(int, const QString&)));
    window.initialiseWindow();
    int desktopArea = QApplication::desktop()->width() * QApplication::desktop()->height();
    int windowArea = window.width() * window.height();
    if (((float)windowArea / desktopArea) < 0.75f)
        window.show();
    else
        window.showMaximized();

    // Close splash screen.
    splashScreen.finish(&window);

    return app.exec();
}
