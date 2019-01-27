#ifndef FORMMATERIALINDICESMANAGER_H
#define FORMMATERIALINDICESMANAGER_H

#include <QMainWindow>
#include <QOpenGLWidget>
#include <QImage>
#include <QString>
#include <QRgb>

#include "imagebasewidget.h"
#include "formimageprop.h"
#include "openglframebufferobjectproperties.h"

namespace Ui
{
class FormMaterialIndicesManager;
}

typedef std::map<int,QRgb>::iterator it_type;

class FormMaterialIndicesManager : public ImageBaseWidget
{
    Q_OBJECT

public:
    FormMaterialIndicesManager(QMainWindow *parent = 0, QOpenGLWidget *qlW_ptr = 0 );
    ~FormMaterialIndicesManager();

    void setImage(QImage image);

    // Counts colors and manages material masking
    bool updateMaterials(QImage &_image);
    bool isEnabled();
    void disableMaterials();

    // just pointers to images
    FormImageProp* imagesPointers[7];

public slots:
    void changeMaterial(int index);
    void pasteFromClipboard();
    void copyToClipboard();
    // Enable/disable materials.
    void toggleMaterials(bool toggle);
    // Takes a color then searches for similar in materials table.
    void chooseMaterialByColor(QColor color);

signals:
    void materialChanged();
    void imageLoaded(int width,int height);
    void materialsToggled(bool);

protected:
    bool loadFile(const QString &fileName);
    void pasteImageFromClipboard(QImage& image);

    // Settings
    std::map<QString, OpenGLFramebufferObjectProperties> materialIndices[7];
    std::map<int,QRgb> colorIndices;
    int lastMaterialIndex;
    Ui::FormMaterialIndicesManager *ui;
    bool bSkipUpdating;
};

#endif // FORMMATERIALINDICESMANAGER_H
