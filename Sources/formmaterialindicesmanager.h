#ifndef FORMMATERIALINDICESMANAGER_H
#define FORMMATERIALINDICESMANAGER_H

#include <QWidget>
#include <QOpenGLWidget>
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
    FormMaterialIndicesManager(QWidget *parent = 0, QOpenGLWidget *qlW_ptr = 0);
    ~FormMaterialIndicesManager();

    Image* getImage(){return &imageProp;}
    void setImage(const QImage &image);

    // Counts colors and manages material masking
    bool updateMaterials(const QImage &image);
    bool isEnabled();
    void disableMaterials();

    // just pointers to images
    ImageWidget* imagesPointers[7];

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
    void pasteImageFromClipboard(const QImage& image);

    // Settings
    std::map<QString, Image> materialIndices[7];
    std::map<int,QRgb> colorIndices;
    int lastMaterialIndex;
    Ui::FormMaterialIndicesManager *ui;
    bool bSkipUpdating;

private:
    Image imageProp;
};

#endif // FORMMATERIALINDICESMANAGER_H
