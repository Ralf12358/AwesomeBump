include(../QtnProperty/QtnProperty.pri)
include(../QtnProperty/PEG.pri)

TEMPLATE = app
TARGET = AwesomeBump
DESTDIR = $$PWD/../Bin
VERSION = 6.0.0

CONFIG  += c++11
QT      += opengl widgets

# Windows requires including opengl library.
win32{
    msvc: LIBS += Opengl32.lib
}

HEADERS = \
    allaboutdialog.h \
    basemapconvlevelproperties.h \
    camera.h \
    dialogheightcalculator.h \
    dialoglogger.h \
    dialogshortcuts.h \
    display3dsettings.h \
    dockwidget3dsettings.h \
    formimagebatch.h \
    formmaterialindicesmanager.h \
    formsettingscontainer.h \
    formsettingsfield.h \
    image.h \
    imagewidget.h \
    mainwindow.h \
    opengl2dimagewidget.h \
    opengl3dimagewidget.h \
    openglerrorcheck.h \
    opengltexturecube.h \
    postfixnames.h \
    randomtilingmode.h \
    splashscreen.h \
    targaimage.h \
    properties/Dialog3DGeneralSettings.h \
    properties/PropertyABColor.h \
    properties/propertyconstructor.h \
    properties/PropertyDelegateABColor.h \
    utils/DebugMetricsMonitor.h \
    utils/glslparsedshadercontainer.h \
    utils/glslshaderparser.h \
    utils/mesh.h \
    utils/contextinfo/contextwidget.h \
    utils/contextinfo/renderwindow.h \
    utils/tinyobj/tiny_obj_loader.h

SOURCES = \
    allaboutdialog.cpp \
    basemapconvlevelproperties.cpp \
    camera.cpp \
    dialogheightcalculator.cpp \
    dialoglogger.cpp \
    dialogshortcuts.cpp \
    display3dsettings.cpp \
    dockwidget3dsettings.cpp \
    formimagebatch.cpp \
    formmaterialindicesmanager.cpp \
    formsettingscontainer.cpp \
    formsettingsfield.cpp \
    image.cpp \
    imagewidget.cpp \
    main.cpp \
    mainwindow.cpp \
    opengl2dimagewidget.cpp \
    opengl3dimagewidget.cpp \
    opengltexturecube.cpp \
    postfixnames.cpp \
    randomtilingmode.cpp \
    splashscreen.cpp \
    targaimage.cpp \
    properties/Dialog3DGeneralSettings.cpp \
    properties/PropertyABColor.cpp \
    properties/PropertyDelegateABColor.cpp \
    properties/propertydelegateabfloatslider.cpp \
    utils/DebugMetricsMonitor.cpp \
    utils/glslparsedshadercontainer.cpp \
    utils/glslshaderparser.cpp \
    utils/mesh.cpp \
    utils/contextinfo/contextwidget.cpp \
    utils/contextinfo/renderwindow.cpp \
    utils/tinyobj/tiny_obj_loader.cc

PEG_SOURCES += \
    properties/Filter3DBloom.pef \
    properties/Filter3DDOF.pef \
    properties/Filter3DLensFlares.pef \
    properties/Filter3DToneMapping.pef \
    properties/Filters3D.pef \
    properties/GLSLParsedFragShader.pef \
    properties/ImageProperties.pef

FORMS += \
    allaboutdialog.ui \
    dialogheightcalculator.ui \
    dialoglogger.ui \
    dialogshortcuts.ui \
    dockwidget3dsettings.ui \
    formimagebatch.ui \
    formmaterialindicesmanager.ui \
    formsettingscontainer.ui \
    formsettingsfield.ui \
    imagewidget.ui \
    mainwindow.ui \
    properties/Dialog3DGeneralSettings.ui

RESOURCES += content.qrc

RC_FILE = resources/icon.rc
ICON = resources/icons/icon.icns

DISTFILES += \
    resources/quad.obj

DEFINES += RESOURCE_BASE=\\\"./\\\"
