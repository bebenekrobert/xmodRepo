#include <QCoreApplication>
#include <qlogging.h>

#include "xmodemreceiver.h"
#include "xmodemsender.h"

int main(int argc, char *argv[]){
    QCoreApplication a(argc, argv);

    if(argc < 4 || argc > 5){
        qDebug() << "incorrect arguments, usage:\n xmode <mode:R|S> <COM#> <FILENAME> <initMode:C|NAK>";
        return 0;
    }
    QObject* xModem = NULL;
    if(strcmp(argv[1], "R")==0) {//receiver
        xModem = new XModemReceiver(argv[2], argv[3], argv[4]);
    } else if(strcmp(argv[1],"S")==0) {//sender
        xModem = new XModemSender(argv[2], argv[3]);
    } else {
        qDebug() << "wrong mode (shoudl be R|S)";
        return 0;
    }
    QObject::connect(xModem, SIGNAL(finished()), &a, SLOT(quit()));

    int ret = a.exec();
    delete xModem;
    return ret;
}
