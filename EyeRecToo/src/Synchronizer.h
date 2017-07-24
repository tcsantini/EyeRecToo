#ifndef SYNCHRONIZER_H
#define SYNCHRONIZER_H

#include <list>

#include <QTimer>
#include <QObject>
#include <QSize>

#include "EyeImageProcessor.h"
#include "FieldImageProcessor.h"

#include "utils.h"

class DataTuple {
public:
    DataTuple(){
        timestamp = maxTimestamp;
        lEye = EyeData();
        rEye = EyeData();
        field = FieldData();
        showGazeEstimationVisualization = false;
    }
    DataTuple(Timestamp timestamp, EyeData lEye, EyeData rEye, FieldData field)
        : timestamp(timestamp),
          lEye(lEye),
          rEye(rEye),
          field(field) {}
    Timestamp timestamp;
    EyeData lEye;
    EyeData rEye;
    FieldData field;
    cv::Mat gazeEstimationVisualization;
    bool showGazeEstimationVisualization;
    QString header() const {
        return QString("sync.timestamp") + gDataSeparator + FieldData().header("field.") + EyeData().header("left.") + EyeData().header("right.");
    }
    QString toQString() {
        return QString::number(timestamp) + gDataSeparator + field.toQString() + lEye.toQString() + rEye.toQString();
    }
};

class Synchronizer : public QObject
{
    Q_OBJECT

public:
    explicit Synchronizer(QObject *parent = 0);
    ~Synchronizer();

signals:
    void newData(DataTuple dataTuple);

public slots:
    void newLeftEyeData(EyeData eyeData);
    void newRightEyeData(EyeData eyeData);
    void newFieldData(FieldData fieldData);
    void updateLists();

private:
    unsigned int maxAgeMs;
    unsigned int safeGuardMs;
    unsigned int maxListSize;
    std::list<EyeData> lEyeList;
    std::list<EyeData> rEyeList;
    std::list<FieldData> fieldList;
    bool updated;
    Timestamp lastSynchronization;

    QTimer *timer;
    template<class T> T* getClosestInTime(Timestamp timestamp, std::list<T> &dataList);
    //template<class T> Timestamp getLatestTimestamp(std::list<T> &dataList);
    //Timestamp getLatestTimestamp();

    template<class T> void removeOld(std::list<T> &dataList);

private slots:
    void synchronize(Timestamp timestamp=0);
};

#endif // SYNCHRONIZER_H
