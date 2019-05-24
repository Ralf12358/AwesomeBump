#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QSplashScreen>

class SplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    explicit SplashScreen(QApplication *app, QWidget *parent = 0);

    int m_progress;
    QApplication *app;

public slots:
    void setProgress(int value);
    void setMessage(const QString &msg);

protected:
    void drawContents(QPainter *painter);
};

#endif // SPLASHSCREEN_H
