#include <QDebug>
#include <unistd.h>
#include <QTimer>
#include <QtSerialPort/QSerialPort>

#include "xmodemsender.h"
#include "common.h"

/**
 * @brief XModemSender::XModemSender constructor
 * @param com - port name
 * @param fileName - file name
 */
XModemSender::XModemSender(QString portName, QString fileName, QString initCStr) :
                                                    fileName(fileName),
                                                    packetNr(0),
                                                    bytesRead(0) {
    qDebug() << Q_FUNC_INFO;

    if(QString::compare(initCStr, "C") == 0 ) {
        this->initC = true;
    } else {
        this->initC = false;
    }

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

    file = new QFile(fileName);
    if (!file->open(QIODevice::ReadOnly)){
        qDebug() << " nie mozna otworzyc pliku";
        emit finished();
        return;
    }

    //otwieramy port szeroegowy
    port->open(QIODevice::ReadWrite);//tryb r/w

    //czekamy na NAK od reveivera
    qDebug() << "waiting for NAK...";
}

/**
 * @brief XModemSender::~XModemSender
 */
XModemSender::~XModemSender(){
    qDebug() << Q_FUNC_INFO;
    port->close();
    file->close();
    delete file;
}

/**
 * @brief XModemSender::send - send current file part with header and crc
 * sending starts when NAK is received
 */
void XModemSender::send(){
    qDebug() << Q_FUNC_INFO << " packetNr: " << packetNr;
    QByteArray toSend;
    //nagłowek
    toSend.append(XModem::SOH);
    toSend.append(packetNr);
    toSend.append(255-packetNr);

    //file data
    char* fileData = new char[128];
    bytesRead = file->read(fileData, 128);

    toSend.append(fileData,bytesRead);

    //crc
    // gdy R wysyla C to to crc
    // gdy R wysla NAK to suma algebraiczna bitow (checksum)
//    unsigned cha SerilCom:obliczSume(unsigned char * dane)
//    regiter int i;
//    register unsigned char suma = 0;
//    for(i =0; i < 128; i++){
//        suma += *(dane + i);
//    }
//    soh, 1, 256-1, data[128], suma, 0
//    return suma;
    quint16 crc = qChecksum(fileData,bytesRead);//sume algebraiczna 1 bajtowa a nie dopelnienie

    //crc jest 16bitowe, wiec wysylamy na 2 bajtach
    toSend.append((char)crc & 0xFFUL);
    toSend.append((char)((crc & 0xFF00UL) >> 8) );
    //qDebug() << "crc: " << crc << " parts: " << (unsigned char)toSend[toSend.length()-2] << " " << (unsigned char)toSend[toSend.length()-1];

    //wyslanie naglowka z danymi i crc
    port->write(toSend);
    port->flush();
    port->waitForBytesWritten(1000);
}

/**
 * @brief XModemSender::readData - receive response from receiver
 * NAK - retransmit last file part
 * ACK - send next file part
 * CAN - cancel sneding and quit app
 */
void XModemSender::readData(){
    qDebug() << Q_FUNC_INFO;

    //odczytujemt wszystko z portu
    QByteArray data = port->readAll();
    qDebug() << "read " << data.length() << " bytes";

    if(data[0] == XModem::NAK || data[0] == XModem::C){//zaczynamy wysylanie lub wysylamy ponownie ostatnia czesc pliku
        qDebug() << "NAK received - start sending or resend last part";
        file->seek(file->pos()-bytesRead);//cofamy sie o ostatnio odczytana ilosc bajtow, w przypadku rozpoczescia wysylania - seek(0)
        QTimer::singleShot(100, this, SLOT(send()));
    } else if (data[0] == XModem::ACK) { // wysylamy kolejna czesc pliku
        if(!file->atEnd()){//jeszcze nie koniec pliku
            qDebug() << "sending next file part";
            packetNr++;
            QTimer::singleShot(100, this, SLOT(send()));//wysyalnie podczas odbierania powoduje problemy - wysyłamy po 100ms
        } else { //koniec pliku
            QTimer::singleShot(100, this, SLOT(sendEOT()));//wysyalnie podczas odbierania powoduje problemy - wysyłamy po 100ms
        }
    } else if(data[0] == XModem::CAN) { //cancel
        emit finished();
    }
    port->clear();
}

/**
 * @brief XModemSender::sendEOT
 */
void XModemSender::sendEOT(){
    qDebug() << Q_FUNC_INFO;
    QByteArray toSend;
    toSend.append(XModem::EOT);
    port->write(toSend);
    port->flush();
    emit finished();
}
