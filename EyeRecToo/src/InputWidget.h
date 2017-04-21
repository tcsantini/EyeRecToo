#ifndef INPUTWIDGET_H
#define INPUTWIDGET_H

#include "utils.h"

class InputData
{
public:
    // TODO: in the future we will have an enum for the type of data derived
    // (e.g., Left Eye, Right Eye) and make the synchronizer generic
    // (i.e., taking InputData and operating over a vector of these).
    // This would allow us to dynamically add input widgets
    InputData() {
        timestamp = maxTimestamp;
        processingTimestamp = maxTimestamp;
    }
    Timestamp timestamp;
    Timestamp processingTimestamp;
    virtual QString header(QString prefix) const = 0;
    virtual QString toQString() const = 0;
};

class InputWidget
{
    // When we have the generic synchronizer, we should enforce a signal
    // void newData(InputData) here.
public:
    virtual void startRecording() = 0;
    virtual void stopRecording() = 0;
};

#endif // INPUTWIDGET_H
