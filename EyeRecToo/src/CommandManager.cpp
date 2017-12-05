#include "CommandManager.h"

#include <QDebug>

// TODO: make these keys configurable

void CommandManager::keyPress(QKeyEvent *event)
{
	if (event->isAutoRepeat())
		return;
	switch (event->key()) {
		case Qt::Key_C:
		case Qt::Key_PageDown:
			emit toggleCalibration();
			emit enableMarkerCollection();
			break;
		//case Qt::Key_S:
		//case Qt::Key_PageUp:
		//	emit toggleCalibration();
		//	break;
		case Qt::Key_R:
		case Qt::Key_B:
			emit toggleRecording();
			break;
		case Qt::Key_F:
		case Qt::Key_PageUp:
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
			emit toggleCalibration();
			break;
		case Qt::Key_F:
		case Qt::Key_PageUp:
			emit unfreezeCameraImages();
			break;
		default:
			break;
	}
}
