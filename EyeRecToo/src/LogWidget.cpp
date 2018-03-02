#include "LogWidget.h"
#include "ui_LogWidget.h"

LogWidget::LogWidget(QString id, QWidget *parent) :
	ERWidget(id, parent),
    ui(new Ui::LogWidget)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/icons/logWidget.png"));
}

LogWidget::~LogWidget()
{
    delete ui;
}

void LogWidget::appendMessage(const QString &msg)
{
	ui->log->appendPlainText( msg.trimmed() );
	ui->log->verticalScrollBar()->setValue( ui->log->verticalScrollBar()->maximum() );
}

void LogWidget::on_usrMsg_returnPressed()
{
    on_addMsg_clicked();
}

void LogWidget::on_addMsg_clicked()
{
    qInfo() << "[USER]" << ui->usrMsg->text();
    ui->usrMsg->clear();
}
