#include "connectionlistdlg.h"
#include "ui_connectionlistdlg.h"
#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QFileDialog>
#include <QMenu>

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
QMenu* CConnectionListDlg::setupMenu(QWidget* pMain)
{
    QMenu* pBookmarkMenu = new QMenu ( tr ( "&Bookmarks" ), pMain );
    pBookmarkMenu->addAction ( tr ( "&Add Bookmark" ), this,
    +         SLOT ( OnAddBookmark() ) );
    pBookmarkMenu->addSeparator();
    pBookmarkMenu->addAction ( tr ( "&Load Bookmarks..." ), this,
    +         SLOT ( OnLoadBookmarks() ) );
    pBookmarkMenu->addAction ( tr ( "&Save Bookmarks..." ), this,
    +         SLOT ( OnSaveBookmarks() ) );
    return pBookmarkMenu;
}
void CConnectionListDlg::LoadConnectionList(const QString& filename)
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
void CConnectionListDlg::SaveConnectionList(const QString& filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadWrite)) {
        QMessageBox::warning(this,"Error!",file.errorString());
        return ;
    }
    QTextStream stream( &file );
    for (int var = 0; var < vecServerInfo.size() ; ++var) {
        stream << vecServerInfo[var].strName << ","
        << vecServerInfo[var].HostAddr.toString() << endl;
    }
    file.close();
}
void CConnectionListDlg::OnAddBookmark()
{
    NetworkUtil networkUtil;
    CHostAddress address;
    networkUtil.ParseNetworkAddress(strCurrentServerAddress,address);
    // add server information to vector
    vecServerInfo.Add (
        CServerInfo ( address,
                      address,
                      strCurrentServerName,
                      QLocale::AnyCountry,
                      "AnyCity",
                      0,
                      false ) );
    ui->listWidget->addItem(strCurrentServerName);
    show();
}
void CConnectionListDlg::OnLoadBookmarks()
{
     QString strFileName = QFileDialog::getOpenFileName ( this,
                                                          tr ( "Favourites file" ),
                                                          "",
                                                          QString ( "*.jcl" ));

     if ( !strFileName.isEmpty() )
     {
         // first update the settings struct and then update the mixer panel
         LoadConnectionList(strFileName);
         show();
         raise();
         activateWindow();
     }
}
void CConnectionListDlg::OnSaveBookmarks()
{
     QString strFileName = QFileDialog::getSaveFileName ( this, tr ( "Bookmarks file" ), "", QString ( "*.jcl" ));

     if ( !strFileName.isEmpty() )
     {
         // first update the settings struct and then update the mixer panel
         SaveConnectionList(strFileName);
     }
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

