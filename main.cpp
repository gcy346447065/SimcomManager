#include "mainwindow.h"
#include <QApplication>
#include <QTableWidget>
#include <QTableWidgetItem>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    /*test for tcp
    //QTcpSocket *client = new QTcpSocket();
    //client->connectToHost(QHostAddress("121.42.38.93"), 9898);
    //client->write("hello");
    */

    /*
    QTableWidget *tableWidget = new QTableWidget(10,8);
    tableWidget->setWindowTitle("SimcomManager");
    tableWidget->resize(850, 400);

    tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);//不对表格内容进行修改
    tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);//整行选中的方式
    tableWidget->setAutoScroll(false);//去掉自动滚动

    QStringList header;
    header<<"imei"<<"online/offline"<<"timestamp"<<"latitude"<<"longitude"<<"speed"<<"course"<<"event";
    tableWidget->setHorizontalHeaderLabels(header);
    tableWidget->show();
    */

    return a.exec();
}
