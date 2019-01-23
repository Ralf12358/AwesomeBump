#ifndef FORMSETTINGSCONTAINER_H
#define FORMSETTINGSCONTAINER_H

#include <QWidget>
#include <QDebug>
#include <QDir>
#include "formsettingsfield.h"
#include <QVector>

namespace Ui
{
class FormSettingsContainer;
}

class FormSettingsContainer : public QWidget
{
    Q_OBJECT

public:
    explicit FormSettingsContainer(QWidget *parent = 0);
    ~FormSettingsContainer();

signals:
    // Force main window to read config.ini again.
    void reloadConfigFile();
    // Current configs will be save to config.ini file.
    void forceSaveCurrentConfig();
    // Load settings and convert images
    void emitLoadAndConvert();

public slots:
    // Create a new settings record.
    void addNewSettingsField();
    void toggleAdding();
    // Also destroys files.
    void removeSetting(FormSettingsField *field);
    void reloadSettings(FormSettingsField *);
    void saveSettings();
    void loadAndConvert();
    void filterPresets(QString filter);

private:
    Ui::FormSettingsContainer *ui;
    QVector<FormSettingsField*> settingsList;
};

#endif // FORMSETTINGSCONTAINER_H
