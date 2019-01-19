#ifndef GLWIDGETBASE_H
#define GLWIDGETBASE_H

#include <QGLWidget>
#include <QDebug>

#define KEY_SHOW_MATERIALS Qt::Key_S

class GLWidgetBase : public QGLWidget
{
    Q_OBJECT
public:
    GLWidgetBase(const QGLFormat& format,
                 QWidget *parent=0,
                 QGLWidget *shareWidget=0);
    ~GLWidgetBase();

public:
    void updateGL() Q_DECL_FINAL Q_DECL_OVERRIDE;
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_FINAL Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) ;
    void keyReleaseEvent(QKeyEvent *event) ;

signals:
    void updateGLLater();
    void handleAccumulatedMouseMovementLater();
    void changeCamPositionApplied(bool);

protected:
    virtual void relativeMouseMoveEvent(int dx, int dy, bool* bMouseDragged, Qt::MouseButtons buttons) = 0;
    static bool wrapMouse;

public slots:
    void updateGLNow();
    void toggleMouseWrap(bool toggle);
    void toggleChangeCamPosition(bool toggle);

private slots:
    void handleAccumulatedMouseMovement();

private:
    void handleMovement();

    QPoint lastCursorPos;
    bool updateIsQueued;
    bool mouseUpdateIsQueued;
    bool blockMouseMovement;
    bool eventLoopStarted;

    int dx, dy;
    Qt::MouseButtons buttons;

protected:
    Qt::Key keyPressed;
    QCursor centerCamCursor;
};

#endif // GLWIDGETBASE_H
