#include "CommandManager.h"

#include <QDebug>

// TODO: make these keys configurable

CommandManager::CommandManager(QObject *parent)
{

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
