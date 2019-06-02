#include "dialogheightcalculator.h"
#include "ui_dialogheightcalculator.h"

DialogHeightCalculator::DialogHeightCalculator(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogHeightCalculator)
{
    ui->setupUi(this);
    connect(ui->doubleSpinBoxPhysicalDepth,  SIGNAL(valueChanged(double)), this, SLOT(calculateDepthInPixels(double)));
    connect(ui->doubleSpinBoxPhysicalHeight, SIGNAL(valueChanged(double)), this, SLOT(calculateDepthInPixels(double)));
    connect(ui->doubleSpinBoxPhysicalWidth,  SIGNAL(valueChanged(double)), this, SLOT(calculateDepthInPixels(double)));
    ui->doubleSpinBoxPhysicalWidth ->setValue(1.0);
    ui->doubleSpinBoxPhysicalHeight->setValue(1.0);
    ui->doubleSpinBoxPhysicalDepth ->setValue(0.01);
    ui->doubleSpinBoxPhysicalDepth ->setValue(10.4);
}

DialogHeightCalculator::~DialogHeightCalculator()
{
    delete ui;
}

void DialogHeightCalculator::setImageSize(int width, int height)
{
    ui->spinBoxImageWidth ->setValue(width);
    ui->spinBoxImageHeight->setValue(height);
}

double DialogHeightCalculator::getDepthInPixels()
{
    return ui->doubleSpinBoxDepthInPixels->value();
}

void DialogHeightCalculator::calculateDepthInPixels(double)
{
    double pixelWidth = ui->spinBoxImageWidth ->value();
    double pixelHeight = ui->spinBoxImageHeight->value();
    double physicalWidth = ui->doubleSpinBoxPhysicalWidth ->value();
    double physicalHeight = ui->doubleSpinBoxPhysicalHeight->value();
    double physicalDepth = ui->doubleSpinBoxPhysicalDepth->value();

    // Depth in pixels calculated from width ratio.
    double pixelWidthRatioDepth = 0;
    if(physicalWidth > 1e-05)
        pixelWidthRatioDepth = physicalDepth / physicalWidth * pixelWidth;

    // Depth in pixels calculated from height ratio.
    double pixelHeightRatioDepth = 0;
    if(physicalHeight > 1e-05)
        pixelHeightRatioDepth= physicalDepth / physicalHeight * pixelHeight;

    double depth = (pixelWidthRatioDepth + pixelHeightRatioDepth) / 2;
    ui->doubleSpinBoxDepthInPixels->setValue(depth);
}
