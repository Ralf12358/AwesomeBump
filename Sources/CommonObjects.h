#ifndef COMMONOBJECTS_H
#define COMMONOBJECTS_H

#include <QtOpenGL>
#include <cstdio>
#include <iostream>
#include "qopenglerrorcheck.h"
#include <QOpenGLFunctions_3_3_Core>
#include "targaimage.h"
#include "postfixnames.h"
#include "randomtilingmode.h"
#include "display3dsettings.h"
#include "fboimages.h"
#include "basemapconvlevelproperties.h"
#include "fboimageproperties.h"

#define TAB_SETTINGS 9
#define TAB_TILING   10

#ifdef Q_OS_MAC
# define AB_INI "AwesomeBump.ini"
# define AB_LOG "AwesomeBump.log"
#else
# define AB_INI "config.ini"
# define AB_LOG "log.txt"
#endif

#define KEY_SHOW_MATERIALS Qt::Key_S

//#define USE_OPENGL_330

#ifdef USE_OPENGL_330
#define AWESOME_BUMP_VERSION "AwesomeBump " VERSION_STRING " (2016) (openGL 330 release)"
#else
#define AWESOME_BUMP_VERSION "AwesomeBump " VERSION_STRING " (2016)"
#endif

using namespace std;

enum ConversionType
{
    CONVERT_NONE = 0,
    CONVERT_FROM_H_TO_N,
    CONVERT_FROM_N_TO_H,
    CONVERT_FROM_D_TO_O,    // Diffuse to others.
    CONVERT_FROM_HN_TO_OC,
    CONVERT_RESIZE
};

enum UVManipulationMethods
{
    UV_TRANSLATE = 0,
    UV_GRAB_CORNERS,
    UV_SCALE_XY
};

// Compressed texture type.
enum CompressedFromTypes
{
    H_TO_D_AND_S_TO_N = 0,
    S_TO_D_AND_H_TO_N = 1
};

// Selective blur methods.
enum SelectiveBlurType
{
    SELECTIVE_BLUR_LEVELS = 0,
    SELECTIVE_BLUR_DIFFERENCE_OF_GAUSSIANS
};

enum ColorPickerMethod
{
    COLOR_PICKER_METHOD_A = 0,
    COLOR_PICKER_METHOD_B ,
};

enum MaterialIndicesType
{
    MATERIALS_DISABLED = -10,
    MATERIALS_ENABLED = -1
};

#endif // COMMONOBJECTS_H
