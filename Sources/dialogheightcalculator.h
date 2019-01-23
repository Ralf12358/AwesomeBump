/**
 * Simple class which allows to calculate the depth of the nomral texture
 * based on the height texture and its given physical parameters.
 *
*/
#ifndef DIALOGHEIGHTCALCULATOR_H
#define DIALOGHEIGHTCALCULATOR_H

#include <QDialog>

namespace Ui {
class DialogHeightCalculator;
}

class DialogHeightCalculator : public QDialog
{
    Q_OBJECT

public:
    explicit DialogHeightCalculator(QWidget *parent = 0);
    ~DialogHeightCalculator();
    // Set current image size.
    void setImageSize(int width, int height);
    // Calculate the depth of the normal map based on given parameters.
    float getDepthInPixels();

private slots:
    void calculateDepthInPixels(double);

private:

    Ui::DialogHeightCalculator *ui;
};

#endif // DIALOGHEIGHTCALCULATOR_H
