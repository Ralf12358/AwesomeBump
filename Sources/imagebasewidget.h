#ifndef IMAGEBASEWIDGET_H
#define IMAGEBASEWIDGET_H

#include <QWidget>
#include <QImage>
#include <QString>
#include <QDir>

#include "openglframebufferobjectproperties.h"

// Manages all the input/output operations
// like : open & save from/to file
// paste & copy from/to clipboard
// drag & drop events

class ImageBaseWidget : public QWidget
{
    Q_OBJECT

public:
    ImageBaseWidget(QWidget *parent = 0);
    ~ImageBaseWidget();
    void keyPressEvent(QKeyEvent *event);
    virtual OpenGLFramebufferObjectProperties* getImageProporties(){return &imageProp;}
    virtual void setImageName(QString name);
    virtual QString getImageName();
    virtual void saveFileToDir(const QString &dir);
    virtual void saveImageToDir(const QString &dir,QImage& image);
    virtual void setImageType(TextureTypes imageType);
    OpenGLFramebufferObjectProperties imageProp; // for simplicity I made this public, why not...
    // some properties are visible or hiden for given texture type

protected:
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    virtual void pasteImageFromClipboard(QImage& image) = 0;
    virtual bool loadFile(const QString& file) = 0;
    virtual bool saveFile(const QString &fileName);
    QImage  image;
    QString imageName;

public slots:
    virtual void open();//open dialog
    virtual void save();

public:
    static QDir* recentDir;
};

#endif // IMAGEBASEWIDGET_H
