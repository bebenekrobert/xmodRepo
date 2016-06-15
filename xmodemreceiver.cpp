#include <QDebug>
#include <unistd.h>
#include <QTimer>
#include <QFile>
#include <QtSerialPort/QSerialPort>

#include "xmodemreceiver.h"
#include "common.h"

//flaga sterujaca napisywaniem plikow
#define OVERWRITE_EXISTING_FILES true

/**
 * @brief XModemReceiver::XModemReceiver constructor
 * @param portName - port name
 * @param fileName - file name
 */
XModemReceiver::XModemReceiver(QString portName, QString fileName) :
                                                    fileName(fileName),
                                                    receiving(false) {
    qDebug() << Q_FUNC_INFO;

    //stworzenie s konfigurowanie portu szeregowego
    port = new QSerialPort(this);
    port->setPortName(portName);
    port->setBaudRate(QSerialPort::Baud9600);
    port->setDataBits(QSerialPort::Data8);
    port->setParity(QSerialPort::NoParity);
    port->setStopBits(QSerialPort::OneStop);
    port->setFlowControl(QSerialPort::NoFlowControl);

    //polaczenie sygnalu ze dane zostaly odebrane z portu to naszej funkcji ktora odczyta te dane
    connect(port, SIGNAL(readyRead()), this, SLOT(readData()));

    qDebug() << portName << " " << fileName;

    //otworzenie pliku do zapisu odebranych danych
    file = new QFile(fileName);
    if( file->exists() ){
        if(OVERWRITE_EXISTING_FILES){
            file->open(QIODevice::ReadWrite | QIODevice::Truncate);// zastapiamy istniejacy plik
        } else {
            qDebug() << "Error: File already exists";
            emit finished();
            return;
        }
    } else {
        file->open(QIODevice::ReadWrite);
    }

    //otwieramy port szeroegowy
    port->open(QIODevice::ReadWrite);//tryb r/w

    QTimer::singleShot(100,this,SLOT(initReceive()));//funkcja zostanie wywołana 100ms po wejsciu do mainEventLoop'a
}

/**
 * @brief XModemReceiver::~XModemReceiver destructor
 */
XModemReceiver::~XModemReceiver(){
    qDebug() << Q_FUNC_INFO;
    port->close();
    file->close();
    delete file;
}

/**
 * @brief XModemReceiver::initReceive - initialize receiving
 * sends 6 NAK bytes with 10s interval, if sender doesnt start sending emits finished signal and closes the app
 */
void XModemReceiver::initReceive(){
    static int i = 0;
    if(!receiving && i < 6){
        i++;
        QByteArray toSend;
        toSend.append(XModem::NAK);
        qDebug() << "Sending NAK: " << i;
        port->write(toSend);
        port->flush();
        QTimer::singleShot(10000,this,SLOT(initReceive()));//po 10s wywołaj initReceive ponownie
    } else if(!receiving && i == 6 ){
        qDebug() << "sent NAK 6 times without reply, finishing";
        emit finished();
    }
}

/**
 * @brief XModemReceiver::readData - handle recveived data
 * check the header and crc and write fileData if ok, otherwise request retransmision
 */
void XModemReceiver::readData(){
    qDebug() << Q_FUNC_INFO;
    receiving = true; //przerywa initRequest
    receivedData = port->readAll();//odczytaj dane z portu
    qDebug() << "received : "<<receivedData.length() << " bytes";

    if(receivedData[0] == XModem::SOH){ //jesli StartOfHeader
        //oczytaj reszte naglowka
        unsigned char packetNr = receivedData[1];
        unsigned char complement = receivedData[2];

        //odczytaj crc
        //qDebug() << "crc parts: " << (unsigned char)receivedData[receivedData.length()-2]  << " " << (unsigned char)receivedData[receivedData.length()-1];
        quint16 receivedCrc = (unsigned char)receivedData[receivedData.length()-2] + (int)((unsigned char)receivedData[receivedData.length()-1] << 8);

        qDebug() << "packetNr: " << packetNr  << "complement: " << complement;

        //odczytaj fileData (opuszczamy 3 pierwsze bajty nagłowka i ostatnie dwa z crc)
        QByteArray fileData = receivedData.mid(3,receivedData.length()-5);

        //liczymy crc odebranych danych
        quint16 crc = qChecksum(fileData.data(),fileData.length());

        qDebug() << "receivedCrc: " << receivedCrc;
        qDebug() << "    calcCrc: " << crc;

        if( complement == 255 - packetNr && crc == receivedCrc ){//jest ok
            qDebug() << "complement/packetNr and crc - OK";
            file->write(fileData);
            QTimer::singleShot(100,this, SLOT(sendACK()));//wysyalnie podczas odbierania powoduje problemy - wysyłamy po 100ms
        } else { //bład transmisji
            qDebug() << "transfer error - request resend alt part";
            QTimer::singleShot(100,this, SLOT(sendNAK()));//wysyalnie podczas odbierania powoduje problemy - wysyłamy po 100ms
        }
    } else if(receivedData[0] == XModem::EOT){ //plik zakonczony
        qDebug() << "EOT";
        emit finished();
    }
    port->clear();
}

/**
 * @brief XModemReceiver::sendACK
 */
void XModemReceiver::sendACK(){
    qDebug() << Q_FUNC_INFO;
    QByteArray toSend;
    toSend.append(XModem::ACK);
    port->write(toSend);
}

/**
 * @brief XModemReceiver::sendNAK
 */
void XModemReceiver::sendNAK(){
    qDebug() << Q_FUNC_INFO;
    QByteArray toSend;
    toSend.append(XModem::NAK);
    port->write(toSend);
}
