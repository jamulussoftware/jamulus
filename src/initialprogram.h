#ifndef INITIALPROGRAM_H
#define INITIALPROGRAM_H

#include <QWidget>
#include <QLayout>
#include <QMenuBar>
#include <QTimer>
#include "navegador.h"
#include "ui_serverdlgbase.h"

namespace Ui {
class initialprogram;
}

class initialprogram : public QWidget
{
    Q_OBJECT

public:
    explicit initialprogram(QWidget *parent = nullptr);
    ~initialprogram();

public slots:
    void receiveFromQml();

private slots:
    void on_client_clicked();
    void on_server_clicked();


private:
    Ui::initialprogram *ui;
    QMenu*             pViewMenu;
    QMenuBar*          pMenu;
    QApplication *pApp;
    Navegador *navegador;
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    int m_nMouseClick_X_Coordinate;
    int m_nMouseClick_Y_Coordinate;
};

#endif // INITIALPROGRAM_H
