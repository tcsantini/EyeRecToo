#include "ImageProcessor.h"

ImageProcessor::ImageProcessor(QString id, Type type, QObject *parent)
    : id(id),
      type(type),
      eyeProcessor(NULL),
      fieldProcessor(NULL),
      eyeProcessorUI(NULL),
      fieldProcessorUI(NULL)
{
    Q_UNUSED(parent)
}

ImageProcessor::~ImageProcessor()
{
    switch (type) {
        case Eye:
            if (eyeProcessor)
                eyeProcessor->deleteLater();
            if (eyeProcessorUI)
            eyeProcessorUI->deleteLater();
            break;
        case Field:
            if (fieldProcessor)
                fieldProcessor->deleteLater();
            if (fieldProcessorUI)
                fieldProcessorUI->deleteLater();
            break;
        default:
            break;
    }
}

void ImageProcessor::create()
{
        this->type = type;
        switch (type) {
            case Eye:
                eyeProcessor = new EyeImageProcessor(id);
                connect(this, SIGNAL(process(Timestamp,const cv::Mat&)),
                    eyeProcessor, SLOT(process(Timestamp,const cv::Mat&)) );
                connect(this, SIGNAL(newROI(QPointF,QPointF)),
                    eyeProcessor, SLOT(newROI(QPointF,QPointF)) );

                connect(eyeProcessor, SIGNAL(newData(EyeData)),
                        this, SIGNAL(newData(EyeData)) );

                // GUI
                connect(this, SIGNAL(showOptions(QPoint)),
                        eyeProcessorUI, SLOT(showOptions(QPoint)) );
                eyeProcessorUI->settings = eyeProcessor->settings;
                eyeProcessorUI->pupilDetectionComboBox->addItem("None", "");
                for (int i=0; i<eyeProcessor->availablePupilDetectionMethods.size(); i++) {
                    QString name = eyeProcessor->availablePupilDetectionMethods[i]->description().c_str();
                    eyeProcessorUI->pupilDetectionComboBox->addItem(name, name);
                }
                connect(eyeProcessorUI, SIGNAL(updateConfig()),
                        eyeProcessor, SLOT(updateConfig()) );
                break;
            case Field:
                fieldProcessor = new FieldImageProcessor(id);
                connect(this, SIGNAL(process(Timestamp,const cv::Mat&)),
                    fieldProcessor, SLOT(process(Timestamp,const cv::Mat&)) );
                connect(this, SIGNAL(newROI(QPointF,QPointF)),
                    fieldProcessor, SLOT(newROI(QPointF,QPointF)) );

                connect(fieldProcessor, SIGNAL(newData(FieldData)),
                        this, SIGNAL(newData(FieldData)) );

                connect(this, SIGNAL(showOptions(QPoint)),
                        fieldProcessorUI, SLOT(showOptions(QPoint)) );
                fieldProcessorUI->settings = fieldProcessor->settings;

                connect(fieldProcessorUI, SIGNAL(updateConfig()),
                        fieldProcessor, SLOT(updateConfig()) );
                break;
            default:
                break;
        }
}
