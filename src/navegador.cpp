/**
*    SAGORA
*    @file navegador.cpp
*    @author Juan Manuel López Gargiulo
*    @version 1 26/06/20
*/

#include "navegador.h"

// Renderiza los botones para minimizar y cerrar el widget actual
void Navegador::render(QWidget *widgetContext) {
    window = widgetContext;

    QString qclass = window->metaObject()->className();
    isPantallaInicio = !qclass.compare("initialprogram");
    isPantallaServer = !qclass.compare("CServerDlg");
    isPantallaCliente = !qclass.compare("CClientDlg");



    renderizarBotones();
    formatearVentana();
}

// Minimiza la ventana
void Navegador::minimizar() {

 window->setWindowState(Qt::WindowMinimized);

}

// Cerrar la ventana
void Navegador::cerrar() {
    if (isPantallaInicio) {
        QCoreApplication::quit();
    } else {
        if(isPantallaServer){
            window->setWindowState(Qt::WindowMinimized);
        }else{
            window->close();
        }
    }
}

// Renderiza los botones y los conecta a la correspondiente accion
void Navegador::renderizarBotones() {
    // Seteamos valores para posicionar automaticamente los botones
     int X = window->geometry().size().width() * 0.88;
     int Y = 15;
     int DX = 28;

    if(isPantallaServer){
        X = window->geometry().size().width() * 0.9;
        DX = 26;
    }
    if(isPantallaCliente){
        X = window->geometry().size().width()+135;
        DX = 26;
    }

    btn_min = new QPushButton(window);
    btn_min->setGeometry(X, Y, 16, 16);
    btn_min->setStyleSheet("QPushButton {border-image:  url(:/png/main/res/ui/btn_min.png);}"
                           "QPushButton:hover {border-image:  url(:/png/main/res/ui/btn_min_hover.png);}"
                           "QPushButton:pressed {border-image:  url(:/png/main/res/ui/btn_min_click.png);}");
    btn_min->show();
    btn_close = new QPushButton(window);
    btn_close->setGeometry(X + DX, Y, 16, 16);
    btn_close->setStyleSheet("QPushButton {border-image:  url(:/png/main/res/ui/btn_close.png);}"
                             "QPushButton:hover {border-image:  url(:/png/main/res/ui/btn_close_hover.png);}"
                             "QPushButton:pressed {border-image:  url(:/png/main/res/ui/btn_close_click.png);}");
    btn_close->show();

    connect(btn_min, SIGNAL(clicked()), this, SLOT(minimizar()));
    connect(btn_close, SIGNAL(clicked()), this, SLOT(cerrar()));

}

// Quita la barra de herramientas por defecto y redondea bordes de la ventana
// segun un valor de radio
void Navegador::formatearVentana() {

    window->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    if (isPantallaInicio) {
        const int RADIO = 10;

        QPainterPath path;
        path.addRoundedRect(window->rect(), RADIO, RADIO);
        QRegion mask = QRegion(path.toFillPolygon().toPolygon());
        window->setMask(mask);
    }
}

