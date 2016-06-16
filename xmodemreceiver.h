#ifndef XMODEMRECIEVER_H
#define XMODEMRECIEVER_H

#include <QDebug>
#include <QMessageLogger>
#include <QObject>
#include <QFile>

class QSerialPort;

class XModemReceiver : public QObject{
    Q_OBJECT

signals:
    void finished();

public slots:
    void readData();
    void initReceive();
    void sendACK();
    void sendNAK();

public:
    XModemReceiver(QString portName, QString fileName, QString initCStr);
    ~XModemReceiver();

private:
    QString fileName;
    QFile* file;
    QSerialPort* port;
    bool receiving;
    QByteArray receivedData;
    bool initC;
};

#endif // XMODEMRECIEVER_H
