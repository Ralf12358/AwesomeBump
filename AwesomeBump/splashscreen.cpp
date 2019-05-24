#include "splashscreen.h"

#include <QStyleOptionProgressBar>

SplashScreen::SplashScreen(QApplication *app, QWidget *parent) :
    QSplashScreen(parent), app(app)
{
    setCursor(Qt::BusyCursor);
}

void SplashScreen::setProgress(int value)
{
    m_progress = value;
    if (m_progress > 100)
        m_progress = 100;
    if (m_progress < 0)
        m_progress = 0;
    update();
    repaint();
}

void SplashScreen::setMessage(const QString &msg)
{
    QSplashScreen:: showMessage(msg, Qt::AlignTop);
    update();
    repaint();
}

void SplashScreen::drawContents(QPainter *painter)
{
    QSplashScreen::drawContents(painter);

    // Set style for progressbar...
    QStyleOptionProgressBar pbstyle;
    pbstyle.initFrom(this);
    pbstyle.state = QStyle::State_Enabled;
    pbstyle.textVisible = false;
    pbstyle.minimum = 0;
    pbstyle.maximum = 100;
    pbstyle.progress = m_progress;
    pbstyle.invertedAppearance = false;
    pbstyle.rect = QRect(0, height()-19, width(), 19); // Where is it.

    // Draw it...
    style()->drawControl(QStyle::CE_ProgressBar, &pbstyle, painter, this);
}
