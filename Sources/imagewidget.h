#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QMainWindow>
#include <QImage>
#include <QString>

#include "opengl2dimagewidget.h"
#include "PropertyBase.h"
#include "dialogheightcalculator.h"

namespace Ui
{
class ImageWidget;
}

class ImageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageWidget(QWidget *parent, OpenGL2DImageWidget *openGLWidget, TextureType textureType);
    ~ImageWidget();

    Image* getImage();
    void setImage(const QImage& qImage);
    QString getImageName();
    void setImageName(const QString& name);
    void setOpenGLWidget(QOpenGLWidget *openGLWidget);
    void setupPropertiesGUI();
    void reloadSettings();
    bool loadFile(const QString &fileName);
    void saveFileToDir(const QString& dir);
    void saveImageToDir(const QString& dir, const QImage& qImage);

    static bool loadingImages;
    static QDir* recentDir;

signals:
    void reloadSettingsFromConfigFile(TextureType type);
    void imageChanged();
    void imageLoaded(int width, int height);
    void conversionHeightToNormalApplied();
    void conversionNormalToHeightApplied();
    void conversionBaseConversionApplied();
    void conversionHeightNormalToOcclusionApplied();
    void recalculateOcclusion();
    void toggleColorPickingApplied(bool toggle);
    void pickImageColor( QtnPropertyABColor* property);
    void toggleGrungeSettings(bool toggle);

public slots:
    void reloadImageSettings();
    void copyToClipboard();
    void pasteFromClipboard();
    void pasteNormalFromClipBoard();
    void pasteNormalFromClipBoard(const QtnPropertyButton*);
    void updateComboBoxes(int);
    void updateGuiSpinBoxesAndLabes(int);
    void updateSlidersOnRelease();
    void propertyChanged(const QtnPropertyBase* changedProperty,
                         const QtnPropertyBase*,
                         QtnPropertyChangeReason reason);
    void propertyFinishedEditing();

    // Convert basemap to other textures.
    void applyBaseConversion(const QtnPropertyButton*);
    void applyHeightToNormalConversion();
    void applyNormalToHeightConversion();
    void applyBaseConversionConversion();
    void applyHeightNormalToOcclusionConversion();
    void showHeightCalculatorDialog();
    void pickColorFromImage(QtnPropertyABColor *property);
    void toggleGrungeImageSettingsGroup(bool toggle);
    void loadPredefinedGrunge(QString);

private:
    bool saveFile(const QString &fileName);
    void pasteImageFromClipboard(const QImage &image);

    Image image;
    Ui::ImageWidget *ui;
    // Height calculator tool.
    DialogHeightCalculator *heightCalculator;
};

#endif // IMAGEWIDGET_H
