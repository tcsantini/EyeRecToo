#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QObject>

#include "EyeImageProcessor.h"
#include "FieldImageProcessor.h"

#include "utils.h"

class ImageProcessor : public QObject
{
    Q_OBJECT
public:
    enum Type { Eye, Field };
	explicit ImageProcessor(QString id, Type type, QObject *parent = 0);

    ~ImageProcessor();
	CameraCalibration *cameraCalibration;
	EyeImageProcessorUI* eyeProcessorUI;
	FieldImageProcessorUI* fieldProcessorUI;

signals:
    void process(Timestamp t, cv::Mat frame);
    void showOptions(QPoint pos);
    void newROI(QPointF sROI, QPointF eROI);
    void newData(EyeData data);
    void newData(FieldData data);
	void updateConfig();

public slots:
    void create();

private:
    QString id;
    Type type;
	EyeImageProcessor* eyeProcessor;
	FieldImageProcessor* fieldProcessor;

};

#endif // IMAGEPROCESSOR_H
