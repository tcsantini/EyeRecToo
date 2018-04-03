#ifndef COMMANDMANAGER_H
#define COMMANDMANAGER_H

#include <QObject>
#include <QEvent>
#include <QKeyEvent>

#include "utils.h"

class CommandManager : public QObject
{
	Q_OBJECT

public:
    explicit CommandManager(QObject *parent = 0);

public slots:
	void keyPress(QKeyEvent *event);
	void keyRelease(QKeyEvent *event);

signals:
	void toggleCalibration();
	void toggleMarkerCollection();
	void toggleRemoteCalibration();
	void toggleRecording();
	void toggleRemoteRecording();
	void togglePreview();
	void freezeCameraImages();
    void unfreezeCameraImages();

private:
	int remoteCalibrationToggleKey = Qt::Key_PageDown;
	int calibrationToggleKey = Qt::Key_S;
	int collectionToggleKey = Qt::Key_C;
	int recordingToggleKey = Qt::Key_R;
	int remoteRecordingToggleKey = Qt::Key_PageUp;
	int previewToggleKey = Qt::Key_B;
};

#endif // COMMANDMANAGER_H
