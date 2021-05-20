#include "connectionlistdlg.h"
#include "ui_connectionlistdlg.h"
#include <QFile>
#include <QMessageBox>
#include <QDebug>



CConnectionListDlg::CConnectionListDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CConnectionListDlg),
    vecServerInfo(0),
    ulSelectedServer(0)
{
    ui->setupUi(this);
}

CConnectionListDlg::~CConnectionListDlg()
{
    delete ui;
}
void CConnectionListDlg::LoadConnectionList(QString filename)
{
    NetworkUtil networkUtil;

    ui->listWidget->clear();
    vecServerInfo.clear();

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this,"Error!",file.errorString());
        return ;
    }
    while (!file.atEnd()) {
        QByteArray line = file.readLine();
        QList<QByteArray> items = line.split(',');
        if (items.size() == 2)
        {
        ui->listWidget->addItem(items[0]);
        CHostAddress address;
        networkUtil.ParseNetworkAddress(items[1],address);
        // add server information to vector
        vecServerInfo.Add (
            CServerInfo ( address,
                          address,
                          items[0],
                          QLocale::AnyCountry,
                          "AnyCity",
                          0,
                          false ) );
        } //end if
    } //end while
    file.close();
    ui->listWidget->setCurrentRow(0);
    ulSelectedServer = 0;
}

void CConnectionListDlg::on_connectButton_clicked()
{
    emit ConnectionListDlgUpdated();
}

void CConnectionListDlg::on_nextButton_clicked()
{
    int newRow = ui->listWidget->currentRow()+1;
    if ( newRow == ui->listWidget->count() )
    {
        newRow = 0;
    }
    ulSelectedServer = newRow;
    ui->listWidget->setCurrentRow(newRow);
    ui->listWidget->repaint();
    emit ConnectionListDlgUpdated();
}

QString CConnectionListDlg::getSelectedName()
{
    CServerInfo ci = vecServerInfo[ulSelectedServer];
    return ci.strName;
}

QString CConnectionListDlg::getSelectedAddress()
{
    CServerInfo ci = vecServerInfo[ulSelectedServer];
    return ci.HostAddr.toString();
}

void CConnectionListDlg::on_listWidget_currentRowChanged(int currentRow)
{
    ulSelectedServer = currentRow;
}

void CConnectionListDlg::on_listWidget_itemDoubleClicked(QListWidgetItem *item)
{
    emit ConnectionListDlgUpdated();
}

