#include "ERWidget.h"
#include <QDebug>

ERWidget::ERWidget(QString id, QWidget *parent) :
	id(id),
	QMainWindow(parent)
{
	setDefaults();
}
