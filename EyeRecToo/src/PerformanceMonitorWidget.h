#ifndef PERFORMANCEMONITORWIDGET_H
#define PERFORMANCEMONITORWIDGET_H

#include <vector>

#include <QMainWindow>
#include <QTimer>
#include <QFormLayout>
#include <QLabel>

#include "ERWidget.h"

#include "utils.h"

namespace Ui {
class PerformanceMonitorWidget;
}

class PerformanceMonitorWidget : public ERWidget
{
    Q_OBJECT

public:
	explicit PerformanceMonitorWidget(QString id, QWidget *parent = 0);
	~PerformanceMonitorWidget();

private:
    Ui::PerformanceMonitorWidget *ui;

    QFormLayout *formLayout;
    std::vector< std::pair<QLabel*, QLabel*> > enrolled;
    unsigned int updateTimeMs;

    void clear();
    void fill();

private slots:
    void update();
	void on_resetCounters_clicked();
};

#endif // PERFORMANCEMONITORWIDGET_H
