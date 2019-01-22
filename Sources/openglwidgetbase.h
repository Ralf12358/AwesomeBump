#ifndef OPENGLWIDGETBASE_H
#define OPENGLWIDGETBASE_H

#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPoint>
#include <QCursor>

#define KEY_SHOW_MATERIALS Qt::Key_S

class OpenGLWidgetBase : public QOpenGLWidget
{
    Q_OBJECT

public:
    explicit OpenGLWidgetBase(QWidget *parent = 0);
    ~OpenGLWidgetBase();

public:
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_FINAL Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) ;
    void keyReleaseEvent(QKeyEvent *event) ;

signals:
    void handleAccumulatedMouseMovementLater();
    void changeCamPositionApplied(bool);

protected:
    virtual void relativeMouseMoveEvent(int dx, int dy, bool* bMouseDragged, Qt::MouseButtons buttons) = 0;
    bool wrapMouse;

public slots:
    void toggleMouseWrap(bool toggle);
    void toggleChangeCamPosition(bool toggle);

private slots:
    void handleAccumulatedMouseMovement();

private:
    void handleMovement();

    bool updateIsQueued;
    bool mouseUpdateIsQueued;
    bool blockMouseMovement;
    bool eventLoopStarted;
    int dx, dy;

    QPoint lastCursorPos;
    Qt::MouseButtons buttons;

protected:
    Qt::Key keyPressed;
    QCursor centerCamCursor;
};

#endif // OPENGLWIDGETBASE_H
