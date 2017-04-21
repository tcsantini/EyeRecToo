#ifndef NETWORKSTREAM_H
#define NETWORKSTREAM_H

#include <QObject>
#include <QTimerEvent>
#include <QtNetwork>

#include "Synchronizer.h"

#include "utils.h"

class NetworkStream : public QObject
{
    Q_OBJECT
public:
    explicit NetworkStream(QObject *parent = 0);
    ~NetworkStream();

signals:

public slots:
    void start(int port, QString ip = "broadcast");
    void stop();
    void push(DataTuple dataTuple);

private:
    QTcpServer *tcpServer;
    QAbstractSocket *socket;
    QString ip;
    int port;
};

#endif // NETWORKSTREAM_H
