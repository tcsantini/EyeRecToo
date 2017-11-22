#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include <QObject>
#include <QEvent>
#include <QKeyEvent>

class CommandManager : public QObject
{
	Q_OBJECT

public slots:
	void keyPress(QKeyEvent *event);
	void keyRelease(QKeyEvent *event);

signals:
	void toggleCalibration();
	void toggleRecording();
	void enableMarkerCollection();
	void disableMarkerCollection();
	void freezeCameraImages();
	void unfreezeCameraImages();
};

#endif // COMMANDMANAGER_H
