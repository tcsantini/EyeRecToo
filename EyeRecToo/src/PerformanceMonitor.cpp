#include "PerformanceMonitor.h"

#include <QDebug>

/* NOTICE:
 *
 * We do not use mutexes here for performance reasons so care should be taken.
 * Basic rules:
 *
 * 1) The performance monitor lives throughout the whole program life time,
 * which guarantees valid object access.
 *
 * 2) Once enrolled, there is no removing it. Therefore given indexes are always
 * valid (and should be unique and retrievable for a given id/stage pair!).
 *
 * 3) enrolled.size() == droppedFrameCount.size()
 *
 */

PerformanceMonitor::PerformanceMonitor() :
    frameDropEnabled(true),
    delayedFrameCount(0)
{

}

unsigned int PerformanceMonitor::enrol(const QString &id, const QString &stage)
{
    unsigned int idx = 0;
    QString name = id + " " + stage;

    // Already enrolled?
    for (idx = 0; idx < enrolled.size(); idx++)
        if (enrolled[idx] == name)
            return idx;

    // New enrollment
    enrolled.push_back( name );
    droppedFrameCount.push_back( 0 );

    return idx;
}

bool PerformanceMonitor::shouldDrop(const unsigned int &idx, const unsigned int &delay, const unsigned int &maxDelay)
{
    if (delay > maxDelay) {
        droppedFrameCount[idx]++;
        return frameDropEnabled;
    }
    return false;
}

void PerformanceMonitor::account(const unsigned int &idx)
{
    droppedFrameCount[idx]++;
}

void PerformanceMonitor::report()
{
    qInfo() << "Performance Report:";
    for (unsigned int i = 0; i < enrolled.size(); i++)
        qInfo() << enrolled[i] << "dropped" << droppedFrameCount[i] << "frames.";
}

