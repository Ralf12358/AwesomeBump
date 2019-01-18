#ifndef FORMIMAGEPROP_H
#define FORMIMAGEPROP_H

#include <QWidget>
#include <QImage>
#include <QDir>
#include <QFileDialog>
#include <QDebug>
#include <QMessageBox>
#include <QImageReader>
#include <QStandardPaths>

#include "formimagebase.h"
#include "dialogheightcalculator.h"

namespace Ui
{
class FormImageProp;
}

class FormImageProp : public FormImageBase
{
    Q_OBJECT

public:
    explicit FormImageProp(QMainWindow *parent = 0, QGLWidget* qlW_ptr = 0);
    ~FormImageProp();

    void setImage(QImage image);
    void setPtrToGLWidget(QGLWidget* ptr){ imageProp.glWidget_ptr = ptr;  }
    void setupPopertiesGUI();
    void reloadSettings();
    bool loadFile(const QString &fileName);

public slots:
    void propertyChanged(const QtnPropertyBase* changedProperty,
                         const QtnPropertyBase* firedProperty,
                         QtnPropertyChangeReason reason);
    void propertyFinishedEditing();
    // Convert basemap to other textures.
    void applyBaseConversion(const QtnPropertyButton* button);
    void pasteNormalFromClipBoard(const QtnPropertyButton*);
    void reloadImageSettings();
    void pasteFromClipboard();
    void copyToClipboard();
    void updateComboBoxes(int index);
    void updateGuiSpinBoxesAndLabes(int);
    void updateSlidersOnRelease();
    void applyHeightToNormalConversion();
    void applyNormalToHeightConversion();
    void applyBaseConversionConversion();
    void applyHeightNormalToOcclusionConversion();
    void showHeightCalculatorDialog();
    void pickColorFromImage(QtnPropertyABColor *property);
    void pasteNormalFromClipBoard();
    void toggleGrungeImageSettingsGroup(bool toggle);
    void loadPredefinedGrunge(QString);

signals:
    void reloadSettingsFromConfigFile(TextureTypes type);
    void imageChanged();
    void imageLoaded(int width,int height);
    void conversionHeightToNormalApplied();
    void conversionNormalToHeightApplied();
    void conversionBaseConversionApplied();
    void conversionHeightNormalToOcclusionApplied();
    void recalculateOcclusion();
    void toggleColorPickingApplied(bool toggle);
    void pickImageColor( QtnPropertyABColor* property);
    void toggleGrungeSettings(bool toggle);

private:
    void pasteImageFromClipboard(QImage& _image);

    Ui::FormImageProp *ui;
    // Height calculator tool.
    DialogHeightCalculator *heightCalculator;

public:
    static bool bLoading;
    bool bOpenNormalMapMixer;
};

#endif // FORMIMAGEPROP_H
