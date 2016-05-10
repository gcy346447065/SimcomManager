#include "protocol_manager.h"
#include "mainwindow.h"
#include "eventdialog.h"
#include "finddialog.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDateTime>
#include <QRegExp>

QString gDefaultServer = QString("120.25.157.233:9898");
QString gDefaultMysql = QString("120.25.157.233:3306");

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->lineEdit_Server->setText(gDefaultServer);
    ui->lineEdit_Mysql->setText(gDefaultMysql);

    connect(ui->tableWidget->horizontalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(slotHeaderClicked(int)));

    data_base = QSqlDatabase::addDatabase("QMYSQL"); //add mysql driver
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::findInTableWidget(QString string)
{
    qDebug() << "findInTableWidget:" << string;

    if(ui->tableWidget->findItems(string, Qt::MatchContains).isEmpty())
    {
        qDebug() << "failed to find:" << string;
    }
    else
    {
        qDebug() << "succeed to find:" << string;

        int column = ui->tableWidget->findItems(string, Qt::MatchContains).first()->column();
        int row = ui->tableWidget->findItems(string, Qt::MatchContains).first()->row();

        ui->tableWidget->setCurrentCell(row, column, QItemSelectionModel::Select);
    }

}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if(event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_F)
    {
        qDebug() << "get keyPressEvent ctrl+f";

        FindDialog find(this);
        connect(&find, SIGNAL(findString(QString)), this, SLOT(findInTableWidget(QString)));
        find.exec();
    }
}

void MainWindow::uiShowConnectionStatus(bool connected)
{
    ui->pushButton_Connect->setEnabled(!connected);
    ui->lineEdit_Server->setEnabled(!connected);
    ui->lineEdit_Mysql->setEnabled(!connected);
    ui->pushButton_Disconnect->setEnabled(connected);
    ui->pushButton_GetImeiList->setEnabled(connected);
    ui->pushButton_UpdataImeiData->setEnabled(connected);
    ui->tableWidget->setEnabled(connected);

    return;
}

void MainWindow::on_pushButton_Connect_clicked()
{
    tcpSocket = new QTcpSocket(this);
    connect(tcpSocket,SIGNAL(connected()),this,SLOT(slotConnected()));
    connect(tcpSocket,SIGNAL(disconnected()),this,SLOT(slotDisconnected()));
    connect(tcpSocket,SIGNAL(readyRead()),this,SLOT(slotDataReceived()));

    QString strServer = ui->lineEdit_Server->text();
    QRegExp regServer("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\d{1,5})");
    int iServerPos = regServer.indexIn(strServer);
    if(iServerPos >= 0)
    {
        qDebug() << "connect to server" << regServer.cap(1) << regServer.cap(2);
        tcpSocket->connectToHost(regServer.cap(1), regServer.cap(2).toInt(0, 10));
    }
    else
    {
        qDebug() << "connect to gDefaultServer:" << gDefaultServer;
        ui->lineEdit_Server->setText(gDefaultServer);
        tcpSocket->connectToHost(QString("120.25.157.233"), 9898);//default server
    }

    QString strMysql = ui->lineEdit_Mysql->text();
    QRegExp regMysql("(\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}):(\\d{1,5})");
    int iMysqlPos = regMysql.indexIn(strMysql);
    if(iMysqlPos >= 0)
    {
        QString strMysqlHost = regMysql.cap(1);
        int iMysqlPort = regMysql.cap(2).toInt(0, 10);
        qDebug() << "connect to mysql" << strMysqlHost << iMysqlPort;

        data_base.setHostName(strMysqlHost);
        data_base.setPort(iMysqlPort);
    }
    else
    {
        qDebug() << "connect to gDefaultMysql:" << gDefaultMysql;
        ui->lineEdit_Server->setText(gDefaultMysql);

        data_base.setHostName(QString("120.25.157.233"));
        data_base.setPort(3306);
    }

    data_base.setDatabaseName("gps");
    data_base.setUserName("admin");
    data_base.setPassword("xiaoan2016");
    qDebug() << "want to open mysql";
    if(!data_base.open())
    {
        qDebug() << "connect to mysql failed" << data_base.lastError();
    }
    else
    {
        qDebug() << "connect to mysql success";
    }

    return;
}

void MainWindow::on_pushButton_Disconnect_clicked()
{
    tcpSocket->disconnectFromHost();
    data_base.close();

    ui->tableWidget->setRowCount(0); //clear the table
    return;
}

void MainWindow::on_pushButton_GetImeiList_clicked()
{
    sql_query = QSqlQuery(data_base);
    sql_query.prepare(QString("select imei from object"));
    if(!sql_query.exec())
    {
        qDebug() << sql_query.lastError();
    }
    else
    {
        int iQuerySize = sql_query.size();
        qDebug() << "select imei from object query success" << iQuerySize;
        if(iQuerySize <= 0)
        {
            return;
        }
        else
        {
            ui->tableWidget->setRowCount(0);//clear the table
            ui->label_InProcess_GetImeiList->setText("Geting");
        }

        while(sql_query.next())
        {
            QString imeiString = sql_query.value("imei").toString();

            int rowNum = ui->tableWidget->rowCount();
            ui->tableWidget->setRowCount(rowNum+1);

            ui->tableWidget->setItem(rowNum, 0, new QTableWidgetItem(imeiString));
            ui->tableWidget->setItem(rowNum, 1, new QTableWidgetItem("Details"));
            ui->tableWidget->item(rowNum, 0)->setForeground(Qt::blue);
            ui->tableWidget->item(rowNum, 1)->setForeground(Qt::blue);
        }
        ui->tableWidget->resizeColumnsToContents();
        ui->label_InProcess_GetImeiList->setText("");
    }

    return;
}

void MainWindow::slotConnected()
{
    qDebug() << "slotConnected";

    //modify the buttons
    uiShowConnectionStatus(true);
}

void MainWindow::slotDisconnected()
{
    qDebug() << "slotDisconnected";

    //modify the buttons
    uiShowConnectionStatus(false);
}

void MainWindow::slotHeaderClicked(int column)
{
    qDebug() << "slotHeaderClicked" << column;

    ui->tableWidget->sortByColumn(column);
}

int MainWindow::manager_login(const void *msg)
{
    const MSG_LOGIN_RSP *rsp = (const MSG_LOGIN_RSP *)msg;
    if(ntohs(rsp->length) != sizeof(MSG_LOGIN_REQ) - MSG_HEADER_LEN)
    {
        qDebug("login message length not enough");
        return -1;
    }

    qDebug("get manager login response");
    return 0;
}

void MainWindow::uiShowImeiData(const char *imei, char online_offline, int timestamp, float longitude, float latitude, char speed, short course)
{
    //qDebug("%d,%d,%f,%f,%d,%d",online_offline, timestamp, longitude, latitude, speed, course);
    QDateTime dataTime = QDateTime::fromTime_t(timestamp);
    QString dataTimeString = dataTime.toString("yyyy.MM.dd HH:mm:ss");

    if(ui->tableWidget->findItems(imei, Qt::MatchExactly).isEmpty())
    {
        qDebug() << "can't find items"  << imei;
    }
    else
    {
        qDebug() << "find items" << imei;

        int rowNum = ui->tableWidget->findItems(imei, Qt::MatchExactly).first()->row();

        if(online_offline == 1)
        {
            ui->tableWidget->setItem(rowNum, 2, new QTableWidgetItem("online"));
            ui->tableWidget->item(rowNum, 2)->setForeground(Qt::green);
        }
        else if(online_offline == 2)
        {
            ui->tableWidget->setItem(rowNum, 2, new QTableWidgetItem("offline"));
            ui->tableWidget->item(rowNum, 2)->setForeground(Qt::red);
        }

        ui->tableWidget->setItem(rowNum, 3, new QTableWidgetItem(dataTimeString));
        ui->tableWidget->setItem(rowNum, 4, new QTableWidgetItem(QString::number(longitude, 'f', 6)));
        ui->tableWidget->setItem(rowNum, 5, new QTableWidgetItem(QString::number(latitude, 'f', 6)));
        ui->tableWidget->setItem(rowNum, 6, new QTableWidgetItem(QString("%1").arg((int)speed)));
        ui->tableWidget->setItem(rowNum, 7, new QTableWidgetItem(QString("%1").arg(course)));
        ui->tableWidget->resizeColumnsToContents();
    }

    return;
}

int MainWindow::manager_imeiData(const void *msg)
{
    const MSG_IMEI_DATA_RSP *rsp = (const MSG_IMEI_DATA_RSP *)msg;
    if(ntohs(rsp->header.length) != sizeof(MSG_IMEI_DATA_RSP) - MSG_HEADER_LEN)
    {
        qDebug("imei data message length not enough");
        return -1;
    }

    char imei[MAX_IMEI_LENGTH + 1];
    memcpy(imei, rsp->imei_data.IMEI, MAX_IMEI_LENGTH);
    imei[MAX_IMEI_LENGTH] = '\0'; //add '\0' for string operaton

    char online_offline = rsp->imei_data.online_offline;
    int timestamp = rsp->imei_data.gps.timestamp;
    float longitude = rsp->imei_data.gps.longitude;
    float latitude = rsp->imei_data.gps.latitude;
    char speed = rsp->imei_data.gps.speed;
    short course = rsp->imei_data.gps.course;

    uiShowImeiData(imei, online_offline, timestamp, longitude, latitude, speed, course);

    qDebug() << ntohs(rsp->header.signature) << (unsigned int)(rsp->header.cmd) << (unsigned int)(rsp->header.seq) << ntohs(rsp->header.length);
    if(rsp->header.seq == (char)0xff)
    {
        //updata imei data loop
        int rowNum = ui->tableWidget->findItems(imei, Qt::MatchExactly).first()->row();
        if(rowNum + 1 < ui->tableWidget->rowCount())
        {
            UpdataImeiDataWithRow(rowNum + 1);
        }
        else
        {
            ui->label_InProcess_UpdataImeiData->setText("");
        }
    }

    return 0;
}

int MainWindow::handle_one_msg(const void *m)
{
    const MSG_HEADER *msg = (const MSG_HEADER *)m;

    switch(msg->cmd)
    {
        case CMD_LOGIN:
            return manager_login(msg);

        case CMD_IMEI_DATA:
            return manager_imeiData(msg);

        default:
            return -1;
    }
}

int MainWindow::handle_manager_msg(const char *m, size_t msgLen)
{
    const MSG_HEADER *msg = (const MSG_HEADER *)m;

    if(msgLen < MSG_HEADER_LEN)
    {
        qDebug("message length not enough: %zu(at least(%zu))", msgLen, MSG_HEADER_LEN);

        return -1;
    }
    size_t leftLen = msgLen;

    while(leftLen >= ntohs(msg->length) + MSG_HEADER_LEN)
    {
        const unsigned char *status = (const unsigned char *)(&(msg->signature));
        if((status[0] != 0xaa) || (status[1] != 0x66))
        {
            qDebug("receive message header signature error:%x", (unsigned)ntohs(msg->signature));
        }
        else
        {
            handle_one_msg(msg);
        }

        leftLen = leftLen - MSG_HEADER_LEN - ntohs(msg->length);
        msg = (const MSG_HEADER *)(m + msgLen - leftLen);
    }
    return 0;
}

void MainWindow::slotDataReceived()
{
    qDebug() << tcpSocket->bytesAvailable();
    while(tcpSocket->bytesAvailable() > 0)
    {
        QByteArray datagram;
        datagram.resize(tcpSocket->bytesAvailable());
        tcpSocket->read(datagram.data(), datagram.size());

        handle_manager_msg(datagram.data(), datagram.length());
    }

    return;
}

void MainWindow::on_tableWidget_cellDoubleClicked(int row, int column)
{
    QString imeiString = ui->tableWidget->item(row, 0)->text();
    qDebug() << "on_tableWidget_cellDoubleClicked:" << row << column << imeiString;

    if(column == 0) //get imei data
    {
        QByteArray array_header = QByteArray::fromHex("aa660222000f");
        QByteArray array_imei = QByteArray::fromHex(imeiString.toLatin1().toHex());

        tcpSocket->write(array_header + array_imei);
    }

    if(column == 1) //get event
    {
        EventDialog event(this, imeiString, data_base);
        event.exec();
    }
}

void MainWindow::UpdataImeiDataWithRow(int row)
{
    QString imeiString = ui->tableWidget->item(row, 0)->text(); //get imei string
    qDebug() << "UpdataImeiDataWithRow:" << imeiString;

    QByteArray array_header = QByteArray::fromHex("aa6602ff000f"); //set seq at 0xff for updata imei data loop
    QByteArray array_imei = QByteArray::fromHex(imeiString.toLatin1().toHex());

    tcpSocket->write(array_header + array_imei);
}

void MainWindow::on_pushButton_UpdataImeiData_clicked()
{
    if(ui->tableWidget->rowCount() != 0)
    {
        UpdataImeiDataWithRow(0);
        ui->label_InProcess_UpdataImeiData->setText("Updating");
    }
}
