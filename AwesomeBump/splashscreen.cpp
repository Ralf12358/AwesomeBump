#include "splashscreen.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QStyleOptionProgressBar>

SplashScreen::SplashScreen()
{
    QPixmap pixMap = QPixmap(":/resources/logo/splash.png");
    // Resize splash screen to 1/4 of the screen.
    QSize size = pixMap.size() * (0.25 *
            QApplication::desktop()->screenGeometry().width() /
            pixMap.size().width());
    setPixmap(pixMap.scaled(size));
    showMessage("Starting ...", Qt::AlignTop);
    setCursor(Qt::BusyCursor);
}

void SplashScreen::updateProgress(int progress, const QString& message)
{
    if (progress < 0)   progress = 0;
    if (progress > 100) progress = 100;
    this->progress = progress;

    showMessage(message, Qt::AlignTop);

    update();
    repaint();
}

void SplashScreen::drawContents(QPainter *painter)
{
    QSplashScreen::drawContents(painter);

    // Draw ProgressBar...
    QStyleOptionProgressBar progressBar;
    progressBar.initFrom(this);
    progressBar.state = QStyle::State_Enabled;
    progressBar.minimum = 0;
    progressBar.maximum = 100;
    progressBar.progress = progress;
    // Set location.
    progressBar.rect = QRect(0, height()-20, width(), 20);

    style()->drawControl(QStyle::CE_ProgressBar, &progressBar, painter, this);
}
