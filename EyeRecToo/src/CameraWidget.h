#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include <QMainWindow>
#include <QThread>
#include <QCamera>
#include <QAction>
#include <QMouseEvent>
#include <QPainter>
#include <QFont>
#include <QMessageBox>

#include "ERWidget.h"

#include "opencv2/imgproc.hpp"
#include "opencv2/calib3d.hpp"

#include "Camera.h"
#include "ImageProcessor.h"
#include "EyeImageProcessor.h"
#include "FieldImageProcessor.h"
#include "CameraCalibration.h"

#include "DataRecorder.h"

#include "Synchronizer.h"

#include "utils.h"

#include "Overlay.h"

namespace Ui {
class CameraWidget;
}

class CameraWidget : public ERWidget, InputWidget
{
    Q_OBJECT

public:
    explicit CameraWidget(QString id, ImageProcessor::Type type, QWidget *parent = 0);
	~CameraWidget();

signals:
    void setCamera(QCameraInfo cameraInfo);
    void newROI(QPointF sROI, QPointF eROI);
    void newData(EyeData data);
    void newData(FieldData data);
	void newClick(Timestamp,QPoint,QSize);

public slots:
    void preview(Timestamp t, const cv::Mat &frame);
    void preview(EyeData data);
    void preview(FieldData data);
    void preview(const DataTuple &data);
    void options(QAction* action);
    void noCamera(QString msg);

    void startRecording();
    void stopRecording();

    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);

    void validatePoint(QPointF &point);

	void requestCameraCalibrationSample() { cameraCalibrationSampleRequested = true; }
	void sendCameraCalibrationSample(const cv::Mat &frame);
	void onCameraCalibrationFinished(bool success);

private:
    QString id;
    ImageProcessor::Type type;

    Ui::CameraWidget *ui;

    Camera *camera;
    QThread *cameraThread;

    ImageProcessor *imageProcessor;
    QThread *processorThread;

    DataRecorderThread *recorder;
	QThread *recorderThread;

    QActionGroup *optionsGroup;
    QAction *optionAction;

	std::deque<int> tq;
	Timestamp lastFrameRateUpdate;
    void updateFrameRate(Timestamp t);

    QPointF sROI, eROI;
	bool settingROI;
	void setROI(const QPointF &s, const QPointF &e) { sROI=s; eROI=e; emit newROI(sROI, eROI); }

	Timestamp lastUpdate;
    Timestamp updateIntervalMs;
    Timestamp maxAgeMs;
    bool shouldUpdate(Timestamp t);
    bool isDataRecent(Timestamp t);

    QImage previewImage(const cv::Mat &frame);

	CameraCalibration *cameraCalibration;
	bool cameraCalibrationSampleRequested;

	QSize frameSize = { 0, 0 };
	void updateWidgetSize( const int &width, const int &height);

	// Drawing functionality
	cv::Mat rgb, resized;
	EyeOverlay eyeOverlay;
	FieldOverlay fieldOverlay;

};

#endif // CAMERAWIDGET_H
