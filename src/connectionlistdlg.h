#ifndef CTRLDIALOG_H
#define CTRLDIALOG_H

#include <QDialog>
#include <util.h>
#include <QListWidgetItem>

namespace Ui {
class ConnectionListDlg;
}

class ConnectionListDlg : public QDialog
{
    Q_OBJECT

public:
    explicit ConnectionListDlg(QWidget *parent = nullptr);
    ~ConnectionListDlg();
    QString getSelectedName();
    QString getSelectedAddress();
    void LoadConnectionList(QString filename);

private slots:
    void on_connectButton_clicked();
    void on_nextButton_clicked();
    void on_listWidget_currentRowChanged(int currentRow);
    void on_listWidget_itemDoubleClicked(QListWidgetItem *item);

signals:
    void CtrlDlgUpdated() ;

private:
    Ui::ConnectionListDlg *ui;
    CVector<CServerInfo> vecServerInfo ;
    unsigned long ulSelectedServer;
};

#endif // CTRLDIALOG_H
