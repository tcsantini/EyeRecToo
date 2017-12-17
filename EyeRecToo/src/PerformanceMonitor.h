#ifndef PERFORMANCEMONITOR_H
#define PERFORMANCEMONITOR_H

#include <vector>

#include <QString>

class PerformanceMonitor
{
public:
    PerformanceMonitor();

    std::vector<unsigned long int> droppedFrameCount;
    unsigned int enrol(const QString &id, const QString &stage);
	bool shouldDrop(const unsigned int &idx, const int &delay, const int &maxDelay=50);
    void account(const unsigned int &idx);
    QString getEnrolled(const unsigned int &idx) { return idx < enrolled.size() ? enrolled[idx] : QString(); }
	unsigned int enrolledCount() { return (unsigned int) enrolled.size(); }
	void setFrameDrop(bool enabled = true) { frameDropEnabled = enabled; }

	void report();

private:
    std::vector<QString> enrolled;
    unsigned int delayedFrameCount;
	bool frameDropEnabled;
};

#endif // PERFORMANCEMONITOR_H
