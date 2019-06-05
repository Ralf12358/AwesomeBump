#ifndef FORMMATERIALINDICESMANAGER_H
#define FORMMATERIALINDICESMANAGER_H

#include <QWidget>
#include <QImage>
#include <QString>
#include <QRgb>

#include "imagewidget.h"
#include "image.h"

namespace Ui
{
class FormMaterialIndicesManager;
}

typedef std::map<int,QRgb>::iterator it_type;

class FormMaterialIndicesManager : public QWidget
{
    Q_OBJECT

public:
    FormMaterialIndicesManager(QWidget* parent = 0, OpenGL2DImageWidget* openGL2DImageWidget = 0);
    ~FormMaterialIndicesManager();

    Image* getImage();
    void setImage(const QImage &qImage);

    // Counts colors and manages material masking
    bool updateMaterials(const QImage &qImage);
    bool isEnabled();
    void disableMaterials();

    // just pointers to images
    ImageWidget* imagesPointers[7];

signals:
    void materialChanged();
    void imageLoaded(int width,int height);
    void materialsToggled(bool);

public slots:
    void open();
    void changeMaterial(int index);
    void pasteFromClipboard();
    void copyToClipboard();
    // Enable/disable materials.
    void toggleMaterials(bool toggle);
    // Takes a color then searches for similar in materials table.
    void chooseMaterialByColor(QColor color);

protected:
    bool loadFile(const QString &fileName);
    void pasteImageFromClipboard(const QImage& qImage);

    // Settings
    std::map<QString, Image> materialIndices[7];
    std::map<int,QRgb> colorIndices;
    int lastMaterialIndex;
    Ui::FormMaterialIndicesManager *ui;
    bool bSkipUpdating;

private:
    OpenGL2DImageWidget* openGL2DImageWidget;
    Image image;
};

#endif // FORMMATERIALINDICESMANAGER_H
