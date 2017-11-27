#include "Synchronizer.h"
#include <opencv2/highgui.hpp>

static int gDataTupleId = qRegisterMetaType<DataTuple>("DataTuple");

using namespace std;

Synchronizer::Synchronizer(QObject *parent)
    : QObject(parent),
      maxAgeMs(100),
      safeGuardMs(5),
      maxListSize(5),
      updated(false),
      lastSynchronization(0)
{

    // TODO: allow user to pick sampling rate
    bool userSelected = false;
    if (userSelected) {
        // This may be dangerous and is in practice much less efficient than
        // synchronizing one of the cameras as trigger; e.g., due to timer imprecisions
        int userSynchronizeMs = 8;
        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()),
                this, SLOT(synchronize()));
        timer->start(userSynchronizeMs);
    } else {
        // Hardware-based triggers
        // Synchronization priority: Right Eye, Left Eye, Field
        // E.g., if no right eye is available we use left eye for synchronization and so on
        timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()),
                this, SLOT(updateLists()));
        timer->start(1e3);
    }
}

Synchronizer::~Synchronizer()
{

}

void Synchronizer::newRightEyeData(EyeData eyeData)
{
	rEyeList.emplace_back(eyeData);
    if (rEyeList.size() > maxListSize)
        rEyeList.pop_front();
    updated = true;
    if (!rEyeList.empty())
        synchronize(eyeData.timestamp);
}

void Synchronizer::newLeftEyeData(EyeData eyeData)
{
	lEyeList.emplace_back(eyeData);
    if (lEyeList.size() > maxListSize)
        lEyeList.pop_front();
    updated = true;

    if (rEyeList.empty())
        synchronize(eyeData.timestamp);
}

void Synchronizer::newFieldData(FieldData fieldData)
{
	fieldList.emplace_back(fieldData);
    if (fieldList.size() > maxListSize)
        fieldList.pop_front();
    updated = true;

    if (rEyeList.empty() && lEyeList.empty())
        synchronize(fieldData.timestamp);
}

template<class T>
T* Synchronizer::getClosestInTime(Timestamp timestamp, list<T> &dataList)
{
    T *closest = NULL;
    if (dataList.empty())
        return closest;

    unsigned int minimum = UINT_MAX;
    for (auto it = dataList.begin(); it != dataList.end(); ++it) {
        unsigned int diff = abs( timestamp - it->timestamp);
        if (diff < minimum) {
            minimum = diff;
            closest = &(*it);
        }
    }

    if (minimum > maxAgeMs)
        return NULL;

    return closest;
}

/* Old style synchronization
template<class T>
Timestamp Synchronizer::getLatestTimestamp(list<T> &dataList)
{
    // maintain monotonicity of the synchronized signal in exchange of ignoring some information
    // this may happen because we want to synchronize the data based on their timestamps (i.e., how close in time they are)
    // but they only become available after processing, which takes a significant non-constant amount of time
    if ( dataList.empty() || dataList.back().timestamp <= lastSynchronization)
        return maxTimestamp;
    else
        return dataList.back().timestamp;
}

Timestamp Synchronizer::getLatestTimestamp()
{
    return min<Timestamp>(
                getLatestTimestamp<FieldData>(fieldList),
                min<Timestamp>(
                    getLatestTimestamp<EyeData>(lEyeList),
                    getLatestTimestamp<EyeData>(rEyeList)
                    )
                );
}
*/

void Synchronizer::synchronize(Timestamp timestamp)
{
    if (!updated)
        return;
    updated = false;

    //if (timestamp == 0) // old style
        //timestamp = getLatestTimestamp();
    if (timestamp == 0)
        timestamp = gTimer.elapsed();

    if (timestamp == maxTimestamp) // nothing recent was available
        return;

    // Make sure we don't overload the thread event loop
    if (timestamp - lastSynchronization < safeGuardMs)
        return;

    lastSynchronization = timestamp;

    FieldData *field = getClosestInTime<FieldData>(timestamp, fieldList);
    EyeData *lEye = getClosestInTime<EyeData>(timestamp, lEyeList);
    EyeData *rEye = getClosestInTime<EyeData>(timestamp, rEyeList);

    DataTuple dataTuple (
                timestamp,
                (lEye ? *lEye : EyeData()),
                (rEye ? *rEye : EyeData()),
                (field ? *field : FieldData())
                );

    emit newData(dataTuple);
}

template<class T>
void Synchronizer::removeOld(list<T> &dataList)
{
    if (dataList.empty())
        return;

    if (gTimer.elapsed() - dataList.back().timestamp > maxAgeMs)
        dataList.clear();
}
void Synchronizer::updateLists()
{
    removeOld(rEyeList);
    removeOld(lEyeList);
    removeOld(fieldList);
}
