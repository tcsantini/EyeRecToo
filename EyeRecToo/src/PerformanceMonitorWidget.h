#ifndef PERFORMANCEMONITORWIDGET_H
#define PERFORMANCEMONITORWIDGET_H

#include <vector>

#include <QMainWindow>
#include <QTimer>
#include <QFormLayout>
#include <QLabel>

#include "utils.h"

namespace Ui {
class PerformanceMonitorWidget;
}

class PerformanceMonitorWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit PerformanceMonitorWidget(QWidget *parent = 0);
	~PerformanceMonitorWidget();

signals:
	void closed();

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

protected:
	void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE { Q_UNUSED(event) emit closed(); }
};

#endif // PERFORMANCEMONITORWIDGET_H
