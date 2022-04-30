#pragma once

/******************************************************************************\
 * Copyright (c) 2022
 * Author(s):
 *  Peter Goderie (pgScorpio)
 *
 *  Use this static class to show basic Error, Warning and Info messages
 *
 *  For own created special message boxes you should always use
 *      CMessages::MainForm() as parent parameter
 *  and CMessages::MainFormName() as base of the title parameter
\******************************************************************************/

#include <QString>
#ifndef HEADLESS
#    include <QMessageBox>
#    define tMainform QDialog
#else
#    include <QDebug>
#    define tMainform void
#endif

// html text macro's (for use in message texts)
#define htmlBold( T ) "<b>" + T + "</b>"
#define htmlNewLine() "<br>"

class CMessages
{
protected:
    static tMainform* pMainForm;
    static QString    strMainFormName;

public:
    static QString ToUtf8Printable ( const QString& text ); // Converts html to plain utf8 text

    static void init ( tMainform* theMainForm, QString theMainFormName )
    {
        pMainForm       = theMainForm;
        strMainFormName = theMainFormName;
    }

    static tMainform*     MainForm() { return pMainForm; }
    static const QString& MainFormName() { return strMainFormName; }

    static void ShowError ( QString strError );
    static void ShowWarning ( QString strWarning );
    static void ShowInfo ( QString strInfo );

    // Modal message boxes (wait for user confirmation in gui mode)
    // Returns true if action button is pressed
    // Returns false if abort button is pressed
    //
    // NOTE: The abort button is only present if a non empty strAbortButtonText is given
    //       if this is the case bDefault pre-selects the default Button.
    //
    // NOTE: In HEADLESS mode there is NO wait and always bDefault is returned.

    static bool ShowErrorWait ( QString       strError,
                                const QString strActionButtonText = "Ok",
                                const QString strAbortButtonText  = "",
                                bool          bDefault            = true );
    static bool ShowWarningWait ( QString       strWarning,
                                  const QString strActionButtonText = "Ok",
                                  const QString strAbortButtonText  = "",
                                  bool          bDefault            = true );
    static bool ShowInfoWait ( QString       strInfo,
                               const QString strActionButtonText = "Ok",
                               const QString strAbortButtonText  = "",
                               bool          bDefault            = true );
};
