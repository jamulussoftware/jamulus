/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *  Volker Fischer
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more 
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#include <qapplication.h>
#include "global.h"
#include "llconclientdlg.h"
#include "llconserverdlg.h"
#include "settings.h"


/* Implementation *************************************************************/
/* This pointer is only used for the post-event routine */
QApplication* pApp = NULL;


int main ( int argc, char** argv )
{
    /* check if server or client application shall be started */
    bool bIsClient         = true;
    bool bUseGUI           = true;
    bool bUseServerLogging = false;

    /* QT docu: argv()[0] is the program name, argv()[1] is the first
       argument and argv()[argc()-1] is the last argument */
    if ( argc > 1 )
    {
        std::string strShortOpt;

        /* "-s": start server with GUI enabled */
        strShortOpt = "-s";
        if ( !strShortOpt.compare ( argv[1] ) )
        {
            bIsClient = false;
        }

        /* "-sn": start server with GUI disabled (no GUI used) */
        strShortOpt = "-sn";
        if ( !strShortOpt.compare ( argv[1] ) )
        {
            bIsClient = false;
            bUseGUI   = false;
        }

        /* "-sln": start server with GUI disabled and logging enabled */
        strShortOpt = "-sln";
        if ( !strShortOpt.compare ( argv[1] ) )
        {
            bIsClient         = false;
            bUseGUI           = false;
            bUseServerLogging = true;
        }
    }

    /* Application object */
    QApplication app ( argc, argv, bUseGUI );

    if ( bIsClient )
    {
        /* client */
        // actual client object
        CClient Client;

        // load settings from init-file
        CSettings Settings ( &Client );
        Settings.Load ();

        // GUI object
        CLlconClientDlg ClientDlg ( &Client, 0, 0, FALSE, Qt::WStyle_MinMax );

        /* Set main window */
        app.setMainWidget ( &ClientDlg );
        pApp = &app; /* Needed for post-event routine */

        /* Show dialog */
        ClientDlg.show();
        app.exec();

        /* Save settings to init-file */
        Settings.Save();
    }
    else
    {
        /* server */
        // actual server object
        CServer Server ( bUseServerLogging );

        if ( bUseGUI )
        {
            // GUI object for the server
            CLlconServerDlg ServerDlg ( &Server, 0, 0, FALSE,
                Qt::WStyle_MinMax );

            /* Set main window */
            app.setMainWidget ( &ServerDlg );
            pApp = &app; /* Needed for post-event routine */

            /* Show dialog */
            ServerDlg.show();
            app.exec();
        }
        else
        {
            // only start application without using the GUI
            qDebug ( CAboutDlg::GetVersionAndNameStr ( false ) );
            app.exec();
        }
    }

    return 0;
}

void PostWinMessage ( const _MESSAGE_IDENT MessID, const int iMessageParam,
                      const int iChanNum )
{
    /* In case of simulation no events should be generated */
    if ( pApp != NULL )
    {
        CLlconEvent* LlconEv =
            new CLlconEvent ( MessID, iMessageParam, iChanNum );

        /* Qt will delete the event object when done */
        QThread::postEvent ( pApp->mainWidget(), LlconEv );
    }
}
