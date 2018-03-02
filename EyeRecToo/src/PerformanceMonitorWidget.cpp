#include "PerformanceMonitorWidget.h"
#include "ui_PerformanceMonitorWidget.h"

using namespace std;

PerformanceMonitorWidget::PerformanceMonitorWidget(QString id, QWidget *parent) :
	ERWidget(id, parent),
    updateTimeMs(250),
    formLayout(NULL),
    ui(new Ui::PerformanceMonitorWidget)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/icons/performanceMonitorWidget.png"));

    QTimer::singleShot(updateTimeMs, this, SLOT(update()) );
}

PerformanceMonitorWidget::~PerformanceMonitorWidget()
{
    clear();
    delete ui;
}

void PerformanceMonitorWidget::clear()
{
    if (formLayout) {
        delete formLayout;
        formLayout = NULL;
    }
    for (auto e = enrolled.begin(); e != enrolled.end(); e++) {
        delete e->first;
        delete e->second;
    }
    enrolled.clear();
}

void PerformanceMonitorWidget::fill()
{
    clear();

    formLayout = new QFormLayout();
    for( unsigned int i=0; i<gPerformanceMonitor.enrolledCount(); i++ ) {
        pair<QLabel*, QLabel*> formPair  = {
            new QLabel( gPerformanceMonitor.getEnrolled(i) ),
            new QLabel( QString::number(gPerformanceMonitor.droppedFrameCount[i]) )
        };
        enrolled.push_back( formPair );
        formLayout->addRow( formPair.first, formPair.second );
    }
    ui->droppedFramesBox->setLayout(formLayout);
}

void PerformanceMonitorWidget::update()
{
    if (gPerformanceMonitor.enrolledCount() != enrolled.size())
        fill();
    else
        for( unsigned int i=0; i<gPerformanceMonitor.enrolledCount(); i++ )
            enrolled[i].second->setText( QString::number(gPerformanceMonitor.droppedFrameCount[i]) );
    QTimer::singleShot(updateTimeMs, this, SLOT(update()) );
}

void PerformanceMonitorWidget::on_resetCounters_clicked()
{
    gPerformanceMonitor.report();
    qInfo() << "Resetting counters:";
    for (auto c = gPerformanceMonitor.droppedFrameCount.begin(); c != gPerformanceMonitor.droppedFrameCount.end(); c++)
        *c = 0;
}
