
#pragma once

#include <QDialog>
#include <util.h>
#include <QListWidgetItem>

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
    void LoadConnectionList(QString filename);

private slots:
    void on_connectButton_clicked();
    void on_nextButton_clicked();
    void on_listWidget_currentRowChanged(int currentRow);
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

signals:
    void ConnectionListDlgUpdated() ;

private:
    Ui::CConnectionListDlg *ui;
    CVector<CServerInfo> vecServerInfo ;
    unsigned long ulSelectedServer;
};
