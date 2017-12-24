#include "CommandManager.h"

#include <QSettings>
#include <QDebug>

// TODO: make these keys configurable

CommandManager::CommandManager(QObject *parent)
{
    Q_UNUSED(parent);
    QString settingsFileName = QString("%1/CommandManager").arg(gCfgDir);
    bool hasSettings = QFile::exists(settingsFileName);
    QSettings *settings = new QSettings(settingsFileName, QSettings::IniFormat);
    if (hasSettings) {
        calibrationToggleKey = settings->value("calibrationToggleKey").toInt();
        recordingToggleKey = settings->value("recordingToggleKey").toInt();
        previewToggleKey = settings->value("previewToggleKey").toInt();
    } else {
        settings->setValue("calibrationToggleKey", calibrationToggleKey);
        settings->setValue("recordingToggleKey", recordingToggleKey);
        settings->setValue("previewToggleKey", previewToggleKey);
    }
    settings->deleteLater();

}

void CommandManager::keyPress(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return;

    if (event->key() == calibrationToggleKey) {
        if (calibrating) {
            calibrating = false;
            emit disableMarkerCollection();
            emit toggleCalibration();
        } else {
            calibrating = true;
            emit toggleCalibration();
            emit enableMarkerCollection();
        }
    } else if (event->key() == recordingToggleKey) {
        emit toggleRecording();
    } else if (event->key()  == previewToggleKey ){
        emit togglePreview();
    }
}

void CommandManager::keyRelease(QKeyEvent *event)
{
	if (event->isAutoRepeat())
		return;
}
