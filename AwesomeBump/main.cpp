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

    // Choose proper GUI style from config.ini file.
    QSettings settings("config.ini", QSettings::IniFormat);
    QString guiStyle = settings.value("gui_style").toString();
    if (!guiStyle.isEmpty())
        app.setStyle(QStyleFactory::create(guiStyle));
    // Customize some elements.
    app.setStyleSheet("QGroupBox { font-weight: bold; } ");
    // Load specific settings for GUI same for every preset.
    QSettings guiSettings("Configs/gui.ini", QSettings::IniFormat);
    QPalette palette;
    palette.setColor(QPalette::Shadow,
                     QColor(guiSettings.value("slider_font_color", "#000000").toString()));
    app.setPalette(palette);
    QFont font;
    font.setFamily(font.defaultFamily());
    font.setPixelSize(guiSettings.value("font_size", 10).toInt());
    app.setFont(font);

    // Remove old log file.
    QFile::remove(AB_LOG);

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
