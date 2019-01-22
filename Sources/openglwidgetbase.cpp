#include "openglwidgetbase.h"

OpenGLWidgetBase::OpenGLWidgetBase(QWidget *parent) :
    QOpenGLWidget(parent),
    updateIsQueued(false),
    mouseUpdateIsQueued(false),
    blockMouseMovement(false),
    eventLoopStarted(false),
    dx(0),
    dy(0),
    buttons(0),
    keyPressed((Qt::Key)0)
{
    connect(this, &OpenGLWidgetBase::handleAccumulatedMouseMovementLater, this, &OpenGLWidgetBase::handleAccumulatedMouseMovement, Qt::QueuedConnection);
    setMouseTracking(true);
    setFocusPolicy(Qt::ClickFocus);
    centerCamCursor = QCursor(QPixmap(":/resources/cursors/centerCamCursor.png"));
    wrapMouse = true;
}

OpenGLWidgetBase::~OpenGLWidgetBase()
{
}

void OpenGLWidgetBase::mousePressEvent(QMouseEvent *event)
{
    lastCursorPos = event->pos();

    // Reset the mouse handling state with, to avoid a bad state
    blockMouseMovement = false;
    mouseUpdateIsQueued = false;
}

void OpenGLWidgetBase::mouseMoveEvent(QMouseEvent *event)
{
    if(blockMouseMovement)
    {
        // If the mouse was wrapped manually, ignore all mouse events until
        // the event caused by wrapping itself appears to avoid flickering.
        if(event->pos() != lastCursorPos)
            return;
        blockMouseMovement = false;
    }

    // Accumulate the mouse events
    dx += event->x() - lastCursorPos.x();
    dy += event->y() - lastCursorPos.y();
    buttons |= event->buttons();

    lastCursorPos = event->pos();

    // Don't handle mouse movements directly, instead accumulate all queued mouse
    // movements and execute all at once.
    handleMovement();
}

void OpenGLWidgetBase::handleMovement()
{
    // mouseUpdateIsQueued is used to make sure to only handle the accumulated mouse events once.
    if(mouseUpdateIsQueued == false)
    {
        mouseUpdateIsQueued = true;

        // Queue handling teh accumulated mouse events at a later point in time
        handleAccumulatedMouseMovementLater();
    }
}

void OpenGLWidgetBase::handleAccumulatedMouseMovement()
{
    // Handling all queued mouse events, we can accumulate new events from now on
    mouseUpdateIsQueued = false;
    bool mouseDragged = true;

    relativeMouseMoveEvent(dx, dy, &mouseDragged, buttons);

    dx = 0;
    dy = 0;
    buttons = 0;

    if(wrapMouse && mouseDragged){

        bool changed = false;

        if(lastCursorPos.x() > width()-10)
        {
            lastCursorPos.setX(10);
            changed = true;
        }
        if(lastCursorPos.x() < 10)
        {
            lastCursorPos.setX(width()-10);
            changed = true;
        }

        if(lastCursorPos.y() > height()-10)
        {
            lastCursorPos.setY(10);
            changed = true;
        }
        if(lastCursorPos.y() < 10)
        {
            lastCursorPos.setY(height()-10);
            changed = true;
        }

        if(changed)
        {
            QCursor c = cursor();
            c.setPos(mapToGlobal(lastCursorPos));
            setCursor(c);

            // There will be mouse events, which were queued before the mouse
            // cursor was set. We have to ignore that events to avoid flickering.
            blockMouseMovement = true;
        }
    }
    update();

    eventLoopStarted = true;
}

void OpenGLWidgetBase::toggleMouseWrap(bool toggle)
{
    wrapMouse = toggle;
}

void OpenGLWidgetBase::toggleChangeCamPosition(bool toggle)
{
    if(!toggle)
    {
        setCursor(Qt::PointingHandCursor);
        keyPressed = (Qt::Key)0;
    }
    else
    {
        keyPressed = Qt::Key_Shift;
        setCursor(centerCamCursor);
    }
    update();
}

void OpenGLWidgetBase::keyPressEvent(QKeyEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        // Enable material preview.
        if( event->key() == KEY_SHOW_MATERIALS )
        {
            keyPressed = KEY_SHOW_MATERIALS;
            update();
        }
        if( event->key() == Qt::Key_Shift )
        {
            if(keyPressed == Qt::Key_Shift)
            {
                setCursor(Qt::PointingHandCursor);
                keyPressed = (Qt::Key)0;
                emit changeCamPositionApplied(false);
            }
            else
            {
                keyPressed = Qt::Key_Shift;
                setCursor(centerCamCursor);
            }
            update();
        }

    }
}

void OpenGLWidgetBase::keyReleaseEvent(QKeyEvent *event)
{
    if (event->type() == QEvent::KeyRelease)
    {
        if( event->key() == KEY_SHOW_MATERIALS)
        {
            keyPressed = (Qt::Key)0;
            update();
            event->accept();
        }
    }
}
