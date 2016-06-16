#ifndef XMODEMSENDER_H
#define XMODEMSENDER_H

#include <QDebug>
#include <QObject>
#include <QFile>

class QSerialPort;

class XModemSender : public QObject{
    Q_OBJECT

signals:
    void finished();

public slots:
    void send();
    void readData();
    void sendEOT();

public:
    XModemSender(QString portName, QString fileName);
    ~XModemSender();

private:
    QString fileName;
    QSerialPort* port;
    QFile* file;
    unsigned char packetNr;
    int bytesRead;
    bool initC;
};

#endif // XMODEMSENDER_H
