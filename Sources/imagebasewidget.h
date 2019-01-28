#ifndef IMAGEBASEWIDGET_H
#define IMAGEBASEWIDGET_H

#include <QWidget>
#include <QImage>
#include <QString>
#include <QDir>

#include "image.h"

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
    virtual Image* getImage(){return &imageProp;}
    virtual void setImageName(const QString &name);
    virtual QString getImageName();
    virtual void saveFileToDir(const QString& dir);
    virtual void saveImageToDir(const QString& dir, const QImage& image);
    virtual void setImageType(TextureType imageType);

protected:
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    virtual void pasteImageFromClipboard(const QImage& image) = 0;
    virtual bool loadFile(const QString& file) = 0;
    virtual bool saveFile(const QString &fileName);

public slots:
    virtual void open();//open dialog
    virtual void save();

public:
    static QDir* recentDir;

private:
    Image imageProp;
};

#endif // IMAGEBASEWIDGET_H
