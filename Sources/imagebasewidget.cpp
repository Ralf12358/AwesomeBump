#include "imagebasewidget.h"

#include <QStandardPaths>
#include <QFileDialog>
#include <QDebug>
#include <QDropEvent>
#include <QUrl>
#include <QDragEnterEvent>
#include <QKeyEvent>
#include <QApplication>
#include <QMimeData>
#include <QClipboard>

#include "targaimage.h"

QDir* ImageBaseWidget::recentDir;

ImageBaseWidget::ImageBaseWidget(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
    setFocus();
    setFocusPolicy(Qt::ClickFocus);
    setAcceptDrops(true);
}

ImageBaseWidget::~ImageBaseWidget()
{
}


void ImageBaseWidget::open()
{
    QStringList picturesLocations;
    if(recentDir == NULL )
        picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    else
        picturesLocations << recentDir->absolutePath();

    QFileDialog dialog(
                this,
                tr("Open File"),
                picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.first(),
                tr("All Images (*.png *.jpg  *.tga *.jpeg *.bmp  *.tif);;"
                          "Images (*.png);;"
                          "Images (*.jpg);;"
                          "Images (*.tga);;"
                          "Images (*.jpeg);;"
                          "Images (*.bmp);;"
                          "Images (*.tif);;"
                          "All files (*.*)"));
    dialog.setAcceptMode(QFileDialog::AcceptOpen);

    while (dialog.exec() == QDialog::Accepted && !loadFile(dialog.selectedFiles().first()))
    {}
}

void ImageBaseWidget::saveFileToDir(const QString &dir)
{
    QString fullFileName = dir + "/" +
            imageName +
            PostfixNames::getPostfix(imageProp.getTextureType()) +
            PostfixNames::outputFormat;
    saveFile(fullFileName);
}

void ImageBaseWidget::saveImageToDir(const QString &dir,QImage& image)
{
    QString fullFileName = dir + "/" +
            imageName +
            PostfixNames::getPostfix(imageProp.getTextureType()) +
            PostfixNames::outputFormat;

    qDebug() << "<FormImageProp> save image:" << fullFileName;
    QFileInfo fileInfo(fullFileName);
    (*recentDir).setPath(fileInfo.absolutePath());

    if( PostfixNames::outputFormat.compare(".tga") == 0)
    {
        TargaImage tgaImage;
        tgaImage.write(image,fullFileName);
    }
    else
        image.save(fullFileName);
}

void ImageBaseWidget::setImageName(QString name)
{
    imageName = name;
}

QString ImageBaseWidget::getImageName()
{
    return imageName;
}

void  ImageBaseWidget::setImageType(TextureType imageType)
{
    imageProp.setTextureType(imageType);
}

void ImageBaseWidget::save()
{
    QStringList picturesLocations;
    if(recentDir == NULL )
    {
        picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    }
    else
    {
        QFileInfo fileInfo(recentDir->absolutePath());
        QString fullFileName = fileInfo.absolutePath() + "/" +
                imageName +
                PostfixNames::getPostfix(imageProp.getTextureType()) +
                PostfixNames::outputFormat;
        picturesLocations << fullFileName;
        qDebug() << "<FormImageProp>:: Saving to file:" << fullFileName;
    }

    QFileDialog dialog(
                this,
                tr("Save current image to file"),
                picturesLocations.isEmpty() ? QDir::currentPath() : picturesLocations.first(),
                tr("All images (*.png *.jpg  *.tga *.jpeg *.bmp *.tif);;All files (*.*)"));
    dialog.setDirectory(recentDir->absolutePath());
    dialog.setAcceptMode(QFileDialog::AcceptSave);

    while (dialog.exec() == QDialog::Accepted && !saveFile(dialog.selectedFiles().first()))
    {}
}

bool ImageBaseWidget::saveFile(const QString &fileName)
{
    qDebug() << Q_FUNC_INFO << "image:" << fileName;

    QFileInfo fileInfo(fileName);
    (*recentDir).setPath(fileInfo.absolutePath());
    image = imageProp.getFBOImage();

    if( PostfixNames::outputFormat.compare(".tga") == 0
            || fileInfo.completeSuffix().compare("tga") == 0 )
    {
        TargaImage tgaImage;
        tgaImage.write(image,fileName);
    }
    else
    {
        image.save(fileName);
    }

    return true;
}

void ImageBaseWidget::dropEvent(QDropEvent *event)
{
    QList<QUrl> droppedUrls = event->mimeData()->urls();
    int i = 0;
    QString localPath = droppedUrls[i].toLocalFile();
    QFileInfo fileInfo(localPath);

    loadFile(fileInfo.absoluteFilePath());
    event->acceptProposedAction();
}

void ImageBaseWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if(event->mimeData()->hasText() || event->mimeData()->hasImage())
    {
        event->acceptProposedAction();
    }
}

void ImageBaseWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        int keyInt = keyEvent->key();
        Qt::Key key = static_cast<Qt::Key>(keyInt);
        if(key == Qt::Key_unknown)
        {
            qDebug() << "Unknown key from a macro probably";
            return;
        }

        // Check for a combination of user clicks.
        Qt::KeyboardModifiers modifiers = keyEvent->modifiers();
        if(modifiers & Qt::ShiftModifier)
            keyInt += Qt::SHIFT;
        if(modifiers & Qt::ControlModifier)
            keyInt += Qt::CTRL;
        if(modifiers & Qt::AltModifier)
            keyInt += Qt::ALT;
        if(modifiers & Qt::MetaModifier)
            keyInt += Qt::META;

        QString keySequenceName = QKeySequence(keyInt).toString(QKeySequence::NativeText);
        // Ctrl + V (paste From clipboard)
        if(keySequenceName.compare("Ctrl+V",Qt::CaseInsensitive) == 0)
        {
            const QClipboard *clipboard = QApplication::clipboard();
            const QMimeData *mimeData = clipboard->mimeData();

            if (mimeData->hasImage())
            {
                qDebug() << "<FormImageProp> Image :" +
                            PostfixNames::getTextureName(imageProp.getTextureType()) +
                            " loaded from clipboard.";
                QPixmap pixmap = qvariant_cast<QPixmap>(mimeData->imageData());
                QImage image = pixmap.toImage();
                pasteImageFromClipboard(image);
            }
        } // End of Ctrl + V (paste From clipboard)

        // Ctrl + C (copy To clipboard)
        if(keySequenceName.compare("Ctrl+C",Qt::CaseInsensitive) == 0)
        {
            qDebug() << "<FormImageProp> Image :" +
                        PostfixNames::getTextureName(imageProp.getTextureType()) +
                        " copied to clipboard.";

            QApplication::processEvents();
            image = imageProp.getFBOImage();
            QApplication::clipboard()->setImage(image,QClipboard::Clipboard);
        } // End of Ctrl + C (copy To clipboard)
    }
}
