#include "DataRecorder.h"

using namespace cv;

DataRecorder::DataRecorder(QString prefix, QString header, QObject *parent)
    : prefix(prefix),
      header(header),
      videoWriter(NULL),
      dataFile(NULL),
      dataStream(NULL),
      framerate(0),
      QObject(parent)
{
}

DataRecorder::~DataRecorder()
{
    stopRecording();
}

void DataRecorder::startRecording()
{
    QString fileName = prefix + "Data.csv";
    dataFile = new QFile( fileName );
    if ( !dataFile->open(QIODevice::WriteOnly) ) {
        qWarning() << "Recording failure." << QString("Could not open %1").arg(fileName);
        delete dataFile;
        dataFile = NULL;
    }
    dataStream = new QTextStream(dataFile);
    *dataStream << header << gDataNewline;
}

void DataRecorder::startRecording(double fps)
{
    startRecording();
    videoWriter = new VideoWriter();
    firstFrame = true;
    videoIdx = 1;
    this->fps = fps;
}

void DataRecorder::stopRecording()
{
    if (videoWriter)
        videoWriter->release();

    if (dataFile){
        dataFile->close();
        dataFile->deleteLater();
    }
    delete dataStream;
    delete videoWriter;

    dataStream = NULL;
    dataFile = NULL;
    videoWriter = NULL;
}

void DataRecorder::newData(EyeData eyeData)
{
    storeData(eyeData);
}

void DataRecorder::newData(FieldData fieldData)
{
    storeData(fieldData);
}

void DataRecorder::newData(DataTuple dataTuple)
{
    if (dataStream == NULL)
        return;

    if (!gRecording)
        return;

    if (dataStream->status() == QTextStream::Ok) {
        *dataStream << dataTuple.toQString();
        *dataStream << gDataNewline;
    }
}

bool DataRecorder::splitVideoFile()
{
    currentVideoFileInfo.refresh();
    if (currentVideoFileInfo.size() >= 1001390080 ) // to deal with opencv's avi size limitation (i.e., 2GB)
        return true;
    else
        return false;
}

template <class T>
void DataRecorder::storeData(T &data)
{
    if (firstFrame || splitVideoFile()) {
        firstFrame = false;

        QString fileName = prefix + "-P" + QString::number(videoIdx) + ".avi";

        int codec = videoWriter->fourcc('D', 'I', 'V', 'X');
        if (gHasOpenH264)
            codec = videoWriter->fourcc('x', '2', '6', '4');

        if (videoWriter->isOpened())
            videoWriter->release();

        if (!videoWriter->open(fileName.toStdString(), codec, this->fps, Size(data.input.cols, data.input.rows), data.input.channels() == 1 ? false : true))
            qWarning() << "Recording failure." << QString("Could not open %1").arg(fileName);

        currentVideoFileInfo.setFile(fileName);

        videoIdx++;
    }

    if (!gRecording)
        return;

    if (videoWriter->isOpened())
        *videoWriter << data.input;
    if (dataStream->status() == QTextStream::Ok)
        *dataStream << data.toQString() << gDataNewline;
}
