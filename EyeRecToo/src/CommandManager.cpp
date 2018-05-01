#include "CommandManager.h"

#include <QSettings>
#include <QDebug>

// TODO: make these keys configurable

CommandManager::CommandManager(QObject *parent)
{
    Q_UNUSED(parent);
    QString settingsFileName = QString("%1/CommandManager.ini").arg(gCfgDir);
    bool hasSettings = QFile::exists(settingsFileName);
    QSettings *settings = new QSettings(settingsFileName, QSettings::IniFormat);
    if (hasSettings) {
		remoteCalibrationToggleKey = settings->value("remoteCalibrationToggleKey").toInt();
		remoteRecordingToggleKey = settings->value("remoteRecordingToggleKey").toInt();
		calibrationToggleKey = settings->value("calibrationToggleKey").toInt();
		collectionToggleKey = settings->value("collectionToggleKey").toInt();
		recordingToggleKey = settings->value("recordingToggleKey").toInt();
        previewToggleKey = settings->value("previewToggleKey").toInt();
    } else {
		settings->setValue("remoteCalibrationToggleKey", remoteCalibrationToggleKey);
		settings->setValue("remoteRecordingToggleKey", remoteRecordingToggleKey);
		settings->setValue("calibrationToggleKey", calibrationToggleKey);
		settings->setValue("collectionToggleKey", collectionToggleKey);
		settings->setValue("recordingToggleKey", recordingToggleKey);
        settings->setValue("previewToggleKey", previewToggleKey);
    }
    settings->deleteLater();

}

void CommandManager::keyPress(QKeyEvent *event)
{
    if (event->isAutoRepeat())
        return;

	if (event->key() == remoteCalibrationToggleKey) {
		emit toggleRemoteCalibration();
	} else if (event->key() == remoteRecordingToggleKey) {
		emit toggleRemoteRecording();
	} else if (event->key() == calibrationToggleKey) {
		emit toggleCalibration();
	} else if (event->key() == collectionToggleKey) {
		emit toggleMarkerCollection();
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
