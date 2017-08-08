#include "LogWidget.h"
#include "ui_LogWidget.h"

LogWidget::LogWidget(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LogWidget)
{
    ui->setupUi(this);

    setWindowTitle("Log Widget");
    setWindowIcon(QIcon(":/icons/logWidget.png"));
}

LogWidget::~LogWidget()
{
    delete ui;
}

void LogWidget::appendMessage(const QString &msg)
{
    ui->log->appendPlainText(msg.trimmed());
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
