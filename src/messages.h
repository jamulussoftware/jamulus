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

    static QString ToUtf8Printable ( const QString& text );

public:
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
};
