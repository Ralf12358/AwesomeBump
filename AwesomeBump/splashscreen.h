#ifndef SPLASHSCREEN_H
#define SPLASHSCREEN_H

#include <QSplashScreen>

class SplashScreen : public QSplashScreen
{
    Q_OBJECT

public:
    SplashScreen();

public slots:
    void updateProgress(int percent, const QString& message);

protected:
    void drawContents(QPainter *painter) override;

private:
    int progress;
};

#endif // SPLASHSCREEN_H
