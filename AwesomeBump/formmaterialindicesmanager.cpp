#include "formmaterialindicesmanager.h"
#include "ui_formmaterialindicesmanager.h"

#include <QDebug>
#include <QMessageBox>
#include <QClipboard>
#include <QMimeData>
#include <QImageReader>

#include "image.h"
#include "opengl2dimagewidget.h"

FormMaterialIndicesManager::FormMaterialIndicesManager(QWidget* parent, OpenGL2DImageWidget* openGL2DImageWidget) :
    QWidget(parent),
    ui(new Ui::FormMaterialIndicesManager),
    openGL2DImageWidget(openGL2DImageWidget)
{
    ui->setupUi(this);

    connect(ui->pushButtonOpenMaterialImage, SIGNAL (released()), this, SLOT (open()));
    connect(ui->pushButtonCopyToClipboard, SIGNAL (released()), this, SLOT (copyToClipboard()));
    connect(ui->pushButtonPasteFromClipboard, SIGNAL (released()), this, SLOT (pasteFromClipboard()));
    connect(ui->checkBoxDisableMaterials, SIGNAL (toggled(bool)), this, SLOT (toggleMaterials(bool)));
    connect(ui->listWidgetMaterialIndices, SIGNAL (currentRowChanged(int)), this, SLOT (changeMaterial(int)));

    ui->groupBox->setDisabled(true);
    setAcceptDrops(true);
}

FormMaterialIndicesManager::~FormMaterialIndicesManager()
{
    qDebug() << "calling" << Q_FUNC_INFO;
    delete ui;
}

bool FormMaterialIndicesManager::isEnabled()
{
    return Image::materialsEnabled;
}

void FormMaterialIndicesManager::disableMaterials()
{
    Image::materialsEnabled = false;
}

Image* FormMaterialIndicesManager::getImage()
{
    return &image;
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
            QtnPropertySetFormImageProp tmp;
            tmp.copyValues(imagesPointers[i]->getProperties());
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

void FormMaterialIndicesManager::open()
{
    QStringList picturesLocations;
    //if(recentDir == NULL)
        picturesLocations = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    //else
    //    picturesLocations << recentDir->absolutePath();

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

    while (dialog.exec() == QDialog::Accepted &&
           !loadFile(dialog.selectedFiles().first()))
    {}
}

void FormMaterialIndicesManager::changeMaterial(int index)
{
    if(bSkipUpdating) return;

    // Copy current settings.
    ui->listWidgetMaterialIndices->item(lastMaterialIndex)->setText("Material"+QString::number(lastMaterialIndex+1));

    QString m_name = ui->listWidgetMaterialIndices->item(lastMaterialIndex)->text();
    for(int i = 0 ; i < MATERIAL_TEXTURE ; i++)
    {
        materialIndices[i][m_name].copyValues(imagesPointers[i]->getProperties());
    }

    lastMaterialIndex = index;

    // Update current mask color.
    QColor bgColor = ui->listWidgetMaterialIndices->item(lastMaterialIndex)->backgroundColor();
    Image::currentMaterialIndex = bgColor.red()*255*255 + bgColor.green()*255 + bgColor.blue();

    // Load different material.
    m_name = ui->listWidgetMaterialIndices->item(index)->text();
    for(int i = 0 ; i < MATERIAL_TEXTURE ; i++)
    {
        imagesPointers[i]->getProperties()->copyValues(&materialIndices[i][m_name]);
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

    QImageReader loadedImage(fileName);
    qImage = loadedImage.read();

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
        openGL2DImageWidget->setTextureImage(MATERIAL_TEXTURE, qImage);
        emit materialChanged();
        Image::currentMaterialIndex = mIndex;
        emit imageLoaded(qImage.width(), qImage.height());
        // Repaint all materials.
        if(Image::materialsEnabled)
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
        openGL2DImageWidget->setTextureImage(MATERIAL_TEXTURE, qImage);
        emit materialChanged();
        Image::currentMaterialIndex = mIndex;
        emit imageLoaded(qImage.width(), qImage.height());
        // Repaint all materials.
        if(Image::materialsEnabled)
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
        Image::materialsEnabled = false;
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
    if(!Image::materialsEnabled) return;

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
        QPixmap pixmap = qvariant_cast<QPixmap>(mimeData->imageData());
        QImage qImage = pixmap.toImage();
        pasteImageFromClipboard(qImage);
    }
}

void FormMaterialIndicesManager::copyToClipboard()
{
    QApplication::processEvents();
    QImage qImage = openGL2DImageWidget->getTextureFBOImage(MATERIAL_TEXTURE);
    QApplication::clipboard()->setImage(qImage,QClipboard::Clipboard);
}
