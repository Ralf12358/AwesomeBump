#include <QApplication>
#include <QSurfaceFormat>
#include <QMessageBox>

#include "mainwindow.h"
#include "splashscreen.h"

#define SplashImage ":/resources/logo/splash.png"

// Register delegates.
extern void regABSliderDelegates();
extern void regABColorDelegates();

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    regABSliderDelegates();
    regABColorDelegates();

    // Create splash screen.
    SplashScreen splashScreen(&app);
    QPixmap splashScreenPixMap = QPixmap(SplashImage);
    // Resize splash screen to 1/4 of the screen.
    QSize splashScreenSize = splashScreenPixMap.size() *
            float(QApplication::desktop()->screenGeometry().width()) / 4.0 /
            float(splashScreenPixMap.size().width());
    splashScreen.resize(splashScreenSize);
    splashScreen.setPixmap(splashScreenPixMap.scaled(splashScreenSize));
    splashScreen.setMessage(VERSION_STRING "|Starting ...");
    splashScreen.show(); app.processEvents();

    // Check for resource directory.
    QString resourceDirectory = getDataDirectory(RESOURCE_BASE);
    if (!QFileInfo(resourceDirectory + "Configs").isDir() ||
            !QFileInfo(resourceDirectory + "Core").isDir())
    {
#ifdef Q_OS_MAC
        return QMessageBox::critical(
                    0,
                    "Missing runtime files",
                    QString("Missing runtime files\n\n"
                            "Cannot find runtime assets required to run the application (resource path: %1).").arg(resDir)
                    );
#else
        return QMessageBox::critical(
                    0,
                    "Missing runtime files",
                    QString("Cannot find runtime assets required to run the application (resource path: %1).").arg(resourceDirectory)
                    );
#endif
    }

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
    //surfaceFormat.setVersion( GL_MAJOR, GL_MINOR );
    QSurfaceFormat::setDefaultFormat(surfaceFormat);

    // Create main application window.
    MainWindow window;
    QObject::connect(&window, SIGNAL (initProgress(int)), &splashScreen, SLOT (setProgress(int)));
    QObject::connect(&window, SIGNAL (initMessage(const QString&)), &splashScreen, SLOT (setMessage(const QString&)));
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
