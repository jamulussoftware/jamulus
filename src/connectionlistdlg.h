
#pragma once

#include <QDialog>
#include <util.h>
#include <QListWidgetItem>
#include <QDebug>

namespace Ui {
class CConnectionListDlg;
}

class CConnectionListDlg : public QDialog
{
    Q_OBJECT

public:
    explicit CConnectionListDlg(QWidget *parent = nullptr);
    ~CConnectionListDlg();
    QString getSelectedName();
    QString getSelectedAddress();
    QMenu* setupMenu(QWidget* pMain);
    inline void setCurrentServer(const QString& name, const QString& address)
    { strCurrentServerName = name; strCurrentServerAddress = address; }
private:
    void LoadConnectionList(const QString& filename);
    void SaveConnectionList(const QString& filename);
private slots:
    void on_connectButton_clicked();
    void on_nextButton_clicked();
    void on_listWidget_currentRowChanged(int currentRow);
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

public slots:
    void OnAddBookmark();
    void OnLoadBookmarks();
    void OnSaveBookmarks();
signals:
    void ConnectionListDlgUpdated() ;

private:
    Ui::CConnectionListDlg *ui;
    CVector<CServerInfo> vecServerInfo ;
    unsigned long ulSelectedServer;
    QString strCurrentServerName;
    QString strCurrentServerAddress;
};
