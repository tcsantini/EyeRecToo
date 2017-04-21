#ifndef DATARECORDER_H
#define DATARECORDER_H

#include <QObject>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

#include <opencv/cv.hpp>

#include "Synchronizer.h"

#include "utils.h"

class DataRecorder : public QObject
{
    Q_OBJECT
public:
    explicit DataRecorder(QString prefix, QString header, QObject *parent = 0);
    ~DataRecorder();

signals:

public slots:
    void startRecording(double fps);
    void startRecording();
    void stopRecording();
    void newData(EyeData eyeData);
    void newData(FieldData fieldData);
    void newData(DataTuple dataTuple);

private:
    QString prefix;
    QString header;
    QFile *dataFile;
    QTextStream *dataStream;
    double framerate;
    cv::VideoWriter *videoWriter;
    bool firstFrame;
    unsigned int videoIdx;
    QFileInfo currentVideoFileInfo;

    template <class T>
    void storeData(T &data);
    bool splitVideoFile();
    double fps;
};

class DataRecorderThread : public QObject
{
    Q_OBJECT
public:
    explicit DataRecorderThread(QString prefix, QString header, QObject *parent = 0)
        : prefix(prefix),
          header(header),
          dataRecorder(NULL),
          QObject(parent)
    {}

    ~DataRecorderThread()
    {
        if (dataRecorder)
            dataRecorder->deleteLater();
        dataRecorder = NULL;
    }

    DataRecorder *dataRecorder;

signals:
    void startRecording(double fps);
    void startRecording();
    void stopRecording();
    void newData(EyeData eyeData);
    void newData(FieldData fieldData);
    void newData(DataTuple dataTuple);

public slots:
    void create() {
        dataRecorder = new DataRecorder(prefix, header, this);
        connect(this, SIGNAL(startRecording(double)),
                dataRecorder, SLOT(startRecording(double)) );
        connect(this, SIGNAL(startRecording()),
                dataRecorder, SLOT(startRecording()) );
        connect(this, SIGNAL(stopRecording()),
                dataRecorder, SLOT(stopRecording()) );
        connect(this, SIGNAL(newData(DataTuple)),
                dataRecorder, SLOT(newData(DataTuple)) );
        connect(this, SIGNAL(newData(EyeData)),
                dataRecorder, SLOT(newData(EyeData)) );
        connect(this, SIGNAL(newData(FieldData)),
                dataRecorder, SLOT(newData(FieldData)) );
    }

private:
    QString prefix;
    QString header;

};

#endif // DATARECORDER_H
