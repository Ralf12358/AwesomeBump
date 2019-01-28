#include "formmaterialindicesmanager.h"
#include "ui_formmaterialindicesmanager.h"

#include <QDebug>
#include <QMessageBox>
#include <QClipboard>
#include <QMimeData>
#include <QImageReader>

#include "targaimage.h"
#include "image.h"

FormMaterialIndicesManager::FormMaterialIndicesManager(QWidget *parent, OpenGL2DImageWidget *openGL2DImageWidget) :
    QWidget(parent),
    ui(new Ui::FormMaterialIndicesManager)
{
    ui->setupUi(this);
    image.setOpenGL2DImageWidget(openGL2DImageWidget);

    connect(ui->pushButtonOpenMaterialImage, SIGNAL (released()), this, SLOT (open()));
    connect(ui->pushButtonCopyToClipboard, SIGNAL (released()), this, SLOT (copyToClipboard()));
    connect(ui->pushButtonPasteFromClipboard, SIGNAL (released()), this, SLOT (pasteFromClipboard()));
    connect(ui->checkBoxDisableMaterials, SIGNAL (toggled(bool)), this, SLOT (toggleMaterials(bool)));
    connect(ui->listWidgetMaterialIndices, SIGNAL (currentRowChanged(int)), this, SLOT (changeMaterial(int)));

    ui->groupBox->setDisabled(true);
    setAcceptDrops(true);
    image.setTextureType(MATERIAL_TEXTURE);
}

FormMaterialIndicesManager::~FormMaterialIndicesManager()
{
    qDebug() << "calling" << Q_FUNC_INFO;
    delete ui;
}

bool FormMaterialIndicesManager::isEnabled()
{
    return (Image::currentMaterialIndex != MATERIALS_DISABLED);
}

void FormMaterialIndicesManager::disableMaterials()
{
    Image::currentMaterialIndex = MATERIALS_DISABLED;
}

Image* FormMaterialIndicesManager::getImage()
{
    return &image;
}

void FormMaterialIndicesManager::setImage(const QImage& qImage)
{
    if (image.getOpenGL2DImageWidget()->isValid())
    {
        // Remember the last id.
        int mIndex = Image::currentMaterialIndex;
        if(updateMaterials(qImage))
        {
            image.init(qImage);
            emit materialChanged();
        }

        Image::currentMaterialIndex = mIndex;
    }
    else
        qDebug() << Q_FUNC_INFO << "Invalid context.";
}

bool FormMaterialIndicesManager::updateMaterials(const QImage& qImage)
{
    bSkipUpdating = true;

    // Calculate image color map.
    colorIndices.clear();
    for(int w = 0 ; w < qImage.width() ; w++)
    {
        for(int h = 0 ; h < qImage.height() ; h++)
        {
            QRgb pixel = qImage.pixel(w,h);
            QColor bgColor = QColor(pixel);
            int index = bgColor.red()*255*255 + bgColor.green()*255 + bgColor.blue();
            colorIndices[index] = pixel;
        }
    }
    if(colorIndices.size() > 32)
    {
        QMessageBox msgBox;
        msgBox.setText("Error: too much colors!");
        msgBox.setInformativeText(" Sorry, but this image does not look like a material texture.\n"
                                  " Your image contains more than 32 different colors");
        msgBox.setStandardButtons(QMessageBox::Cancel);
        msgBox.exec();
        bSkipUpdating = false;
        return false;
    }

    typedef std::map<int,QRgb>::iterator it_type;
    ui->listWidgetMaterialIndices->clear();

    // Generate materials list.
    int indeks = 1;
    for(it_type iterator = colorIndices.begin(); iterator != colorIndices.end(); iterator++)
    {
        qDebug() << "Material index:  " << iterator->first << " Color :" << QColor(iterator->second);

        QListWidgetItem* pItem = new QListWidgetItem("Material"+QString::number(indeks++));
        QColor mColor = QColor(iterator->second);
        // Set text red.
        pItem->setForeground(mColor);
        // Sets background green.
        pItem->setBackground(mColor);
        QColor textColor = QColor(255-mColor.red(),255-mColor.green(),255-mColor.blue());
        pItem->setTextColor(textColor);
        ui->listWidgetMaterialIndices->addItem(pItem);
    }

    qDebug() << "Updating material indices. Total indices count:" << ui->listWidgetMaterialIndices->count();
    for(int i = 0 ; i < MATERIAL_TEXTURE ; i++)
    {
        materialIndices[i].clear();
        for(int m = 0 ; m < ui->listWidgetMaterialIndices->count() ; m++)
        {
            QString m_name = ui->listWidgetMaterialIndices->item(m)->text();
            Image tmp;
            tmp.copySettings(imagesPointers[i]->getImage());
            materialIndices[i][m_name] = tmp;
        }
    }

    lastMaterialIndex = 0;
    QString cText = ui->listWidgetMaterialIndices->item(lastMaterialIndex)->text();
    ui->listWidgetMaterialIndices->item(lastMaterialIndex)->setText(cText+" (selected material)");

    QColor bgColor = ui->listWidgetMaterialIndices->item(lastMaterialIndex)->backgroundColor();
    Image::currentMaterialIndex = bgColor.red()*255*255 + bgColor.green()*255 + bgColor.blue();

    bSkipUpdating = false;

    return true;
}

void FormMaterialIndicesManager::changeMaterial(int index)
{
    if(bSkipUpdating) return;

    // Copy current settings.
    ui->listWidgetMaterialIndices->item(lastMaterialIndex)->setText("Material"+QString::number(lastMaterialIndex+1));

    QString m_name = ui->listWidgetMaterialIndices->item(lastMaterialIndex)->text();
    for(int i = 0 ; i < MATERIAL_TEXTURE ; i++)
    {
        materialIndices[i][m_name].copySettings(imagesPointers[i]->getImage());
    }

    lastMaterialIndex = index;

    // Update current mask color.
    QColor bgColor = ui->listWidgetMaterialIndices->item(lastMaterialIndex)->backgroundColor();
    Image::currentMaterialIndex = bgColor.red()*255*255 + bgColor.green()*255 + bgColor.blue();

    // Load different material.
    m_name = ui->listWidgetMaterialIndices->item(index)->text();
    for(int i = 0 ; i < MATERIAL_TEXTURE ; i++)
    {
        imagesPointers[i]->getImage()->copySettings(&materialIndices[i][m_name]);
        imagesPointers[i]->reloadSettings();
    }

    QString cText = ui->listWidgetMaterialIndices->item(lastMaterialIndex)->text();
    ui->listWidgetMaterialIndices->item(lastMaterialIndex)->setText(cText+" (selected material)");

    emit materialChanged();
}

bool FormMaterialIndicesManager::loadFile(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    QImage qImage;

    // Targa support added.
    if(fileInfo.completeSuffix().compare("tga") == 0)
    {
        TargaImage tgaImage;
        qImage = tgaImage.read(fileName);
    }
    else
    {
        QImageReader loadedImage(fileName);
        qImage = loadedImage.read();
    }

    if (qImage.isNull())
    {
        QMessageBox::information(
                    this, QGuiApplication::applicationDisplayName(),
                    tr("Cannot load material image %1.").arg(QDir::toNativeSeparators(fileName)));
        return false;
    }

    qDebug() << "<FormImageProp> Open material image:" << fileName;

    (*ImageWidget::recentDir).setPath(fileName);

    int mIndex = Image::currentMaterialIndex;
    if(updateMaterials(qImage))
    {
        //image = _image;
        image.init(qImage);
        emit materialChanged();
        Image::currentMaterialIndex = mIndex;
        emit imageLoaded(qImage.width(), qImage.height());
        // Repaint all materials.
        if(Image::currentMaterialIndex != MATERIALS_DISABLED)
        {
            toggleMaterials(true);
        }
    }
    return true;
}

void FormMaterialIndicesManager::pasteImageFromClipboard(const QImage& qImage)
{
    int mIndex = Image::currentMaterialIndex;
    if(updateMaterials(qImage))
    {
        image.init(qImage);
        emit materialChanged();
        Image::currentMaterialIndex = mIndex;
        emit imageLoaded(qImage.width(), qImage.height());
        // Repaint all materials.
        if(Image::currentMaterialIndex != MATERIALS_DISABLED)
        {
            toggleMaterials(true);
        }
    }
}

void FormMaterialIndicesManager::toggleMaterials(bool toggle)
{
    if(toggle == false)
    {
        // Render normally.
        Image::currentMaterialIndex = MATERIALS_DISABLED;
        emit materialChanged();
    }
    else
    {
        // Replot all materials.
        int lastMaterial = lastMaterialIndex;
        // Repaint all materials.
        for(int m = 0 ; m < ui->listWidgetMaterialIndices->count() ; m++)
        {
            QCoreApplication::processEvents();
            changeMaterial(m);
        }
        changeMaterial(lastMaterial);
    }

    emit  materialsToggled(toggle);
}

void FormMaterialIndicesManager::chooseMaterialByColor(QColor color)
{
    // Check if materials are enabled.
    if(Image::currentMaterialIndex == MATERIALS_DISABLED) return;

    bool bColorFound = false;
    // Look for the color in materials.
    for(int m = 0 ; m < ui->listWidgetMaterialIndices->count() ; m++)
    {
        // Get the material color.
        QColor materialColor = ui->listWidgetMaterialIndices->item(m)->backgroundColor();
        int compValue = abs(materialColor.red() - color.red())
                +  abs(materialColor.green() - color.green())
                +  abs(materialColor.blue() - color.blue());

        // If colors are same, change material.
        if(compValue == 0)
        {
            QCoreApplication::processEvents();
            changeMaterial(m);
            bColorFound = true;
            break;
        }

    }

    // If color not found on the list.
    if(!bColorFound)
    {
        QMessageBox::information(
                    this, QGuiApplication::applicationDisplayName(),
                    "The picked color was not found in the materials table\n"
                    "make sure you chose the correct color e.g\n"
                    "not the sky box background.");
    }
}

void FormMaterialIndicesManager::pasteFromClipboard()
{
    const QClipboard *clipboard = QApplication::clipboard();
    const QMimeData *mimeData = clipboard->mimeData();

    if (mimeData->hasImage())
    {
        qDebug() << "<FormImageProp> Image :" +
                    PostfixNames::getTextureName(image.getTextureType()) +
                    " loaded from clipboard.";
        QPixmap pixmap = qvariant_cast<QPixmap>(mimeData->imageData());
        QImage qImage = pixmap.toImage();
        pasteImageFromClipboard(qImage);
    }
}

void FormMaterialIndicesManager::copyToClipboard()
{
    qDebug() << "<FormImageProp> Image :" +
                PostfixNames::getTextureName(image.getTextureType()) +
                " copied to clipboard.";

    QApplication::processEvents();
    QImage qImage = image.getFBOImage();
    QApplication::clipboard()->setImage(qImage,QClipboard::Clipboard);
}
