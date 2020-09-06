/**
*    SAGORA
*    @file navegador.h
*    @author Juan Manuel López Gargiulo
*    @version 1 26/06/20
*/

#ifndef NAVEGADOR_H
#define NAVEGADOR_H

#include <QMouseEvent>
#include <QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <QTimer>


class Navegador: QWidget
{
    Q_OBJECT
public:
    QWidget *window;
    QPushButton *btn_min;
    QPushButton *btn_close;
    bool isPantallaInicio;
    bool isPantallaServer;
    bool isPantallaCliente;

    void render(QWidget *window);
    void renderizarBotones();
    void formatearVentana();
private slots:
    void minimizar();
    void cerrar();
};


#endif // NAVEGADOR_H
