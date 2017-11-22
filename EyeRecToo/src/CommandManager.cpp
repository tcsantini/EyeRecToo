#include "CommandManager.h"

#include <QDebug>

void CommandManager::keyPress(QKeyEvent *event)
{
	if (event->isAutoRepeat())
		return;
	switch (event->key()) {
		case Qt::Key_C:
		case Qt::Key_PageDown:
			emit enableMarkerCollection();
			break;
		case Qt::Key_S:
		case Qt::Key_PageUp:
			emit toggleCalibration();
			break;
		case Qt::Key_R:
			emit toggleRecording();
			break;
		case Qt::Key_F:
			emit freezeCameraImages();
			break;
		default:
			break;
	}
}

void CommandManager::keyRelease(QKeyEvent *event)
{
	if (event->isAutoRepeat())
		return;
	switch (event->key()) {
		case Qt::Key_C:
		case Qt::Key_PageDown:
			emit disableMarkerCollection();
			break;
		case Qt::Key_F:
			emit unfreezeCameraImages();
			break;
		default:
			break;
	}
}
