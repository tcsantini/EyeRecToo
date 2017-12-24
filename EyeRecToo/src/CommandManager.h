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
    void toggleRecording();
    void togglePreview();
	void enableMarkerCollection();
	void disableMarkerCollection();
	void freezeCameraImages();
    void unfreezeCameraImages();

private:
    bool calibrating = false;
    int calibrationToggleKey = Qt::Key_PageDown;
    int recordingToggleKey = Qt::Key_B;
    int previewToggleKey = Qt::Key_PageUp;
};

#endif // COMMANDMANAGER_H
