/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *	Volker Fischer
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

#include "util.h"


/* Implementation *************************************************************/
/* Input level meter implementation ------------------------------------------ */
void CSignalLevelMeter::Update(CVector<double>& vecdAudio)
{
	/* Do the update for entire vector, convert to floating-point */
	const int iVecSize = vecdAudio.Size();

	for (int i = 0; i < iVecSize; i++)
	{
		/* norm of current audio sample */
		const double dCurSig = fabs(vecdAudio[i]);

		/* search for maximum. Decrease this max with time */
		/* decrease max with time */
		if (dCurLevel >= METER_FLY_BACK)
			dCurLevel -= METER_FLY_BACK;
		else
		{
			if ((dCurLevel <= METER_FLY_BACK) && (dCurLevel > 1.0))
				dCurLevel -= 2.0;
		}

		/* search for max */
		if (dCurSig > dCurLevel)
			dCurLevel = dCurSig;
	}
}

double CSignalLevelMeter::MicLevel()
{
	const double dNormMicLevel = dCurLevel / _MAXSHORT;

	/* logarithmic measure */
	if (dNormMicLevel > 0)
		return 20.0 * log10(dNormMicLevel);
	else
		return -100000.0; /* large negative value */
}


/* Global functions implementation ********************************************/
void DebugError(const char* pchErDescr, const char* pchPar1Descr, 
				const double dPar1, const char* pchPar2Descr,
				const double dPar2)
{
	FILE* pFile = fopen("DebugError.dat", "a");
	fprintf(pFile, pchErDescr); fprintf(pFile, " ### ");
	fprintf(pFile, pchPar1Descr); fprintf(pFile, ": ");
	fprintf(pFile, "%e ### ", dPar1);
	fprintf(pFile, pchPar2Descr); fprintf(pFile, ": ");
	fprintf(pFile, "%e\n", dPar2);
	fclose(pFile);
	printf("\nDebug error! For more information see test/DebugError.dat\n");
	exit(1);
}


/******************************************************************************\
* Audio Reverberation                                                          *
\******************************************************************************/
/*
	The following code is based on "JCRev: John Chowning's reverberator class"
	by Perry R. Cook and Gary P. Scavone, 1995 - 2004
	which is in "The Synthesis ToolKit in C++ (STK)"
	http://ccrma.stanford.edu/software/stk

	Original description:
	This class is derived from the CLM JCRev function, which is based on the use
	of networks of simple allpass and comb delay filters. This class implements
	three series allpass units, followed by four parallel comb filters, and two
	decorrelation delay lines in parallel at the output.
*/
CAudioReverb::CAudioReverb(const double rT60)
{
	/* Delay lengths for 44100 Hz sample rate */
	int lengths[9] = {1777, 1847, 1993, 2137, 389, 127, 43, 211, 179};
	const double scaler = (double) SAMPLE_RATE / 44100.0;

	int delay, i;
	if (scaler != 1.0)
	{
		for (i = 0; i < 9; i++)
		{
			delay = (int) floor(scaler * lengths[i]);

			if ((delay & 1) == 0)
				delay++;

			while (!isPrime(delay))
				delay += 2;

			lengths[i] = delay;
		}
	}

	for (i = 0; i < 3; i++)
		allpassDelays_[i].Init(lengths[i + 4]);

	for (i = 0; i < 4; i++)
		combDelays_[i].Init(lengths[i]);

	setT60(rT60);
	allpassCoefficient_ = (double) 0.7;
	Clear();
}

bool CAudioReverb::isPrime(const int number)
{
/*
	Returns true if argument value is prime. Taken from "class Effect" in
	"STK abstract effects parent class".
*/
	if (number == 2)
		return true;

	if (number & 1)
	{
		for (int i = 3; i < (int) sqrt((double) number) + 1; i += 2)
		{
			if ((number % i) == 0)
				return false;
		}

		return true; /* prime */
	}
	else
		return false; /* even */
}

void CAudioReverb::Clear()
{
	/* Reset and clear all internal state */
	allpassDelays_[0].Reset(0);
	allpassDelays_[1].Reset(0);
	allpassDelays_[2].Reset(0);
	combDelays_[0].Reset(0);
	combDelays_[1].Reset(0);
	combDelays_[2].Reset(0);
	combDelays_[3].Reset(0);
}

void CAudioReverb::setT60(const double rT60)
{
	/* Set the reverberation T60 decay time */
	for (int i = 0; i < 4; i++)
	{
		combCoefficient_[i] = pow((double) 10.0, (double) (-3.0 *
			combDelays_[i].Size() / (rT60 * SAMPLE_RATE)));
	}
}

double CAudioReverb::ProcessSample(const double input)
{
	/* Compute one output sample */
	double temp, temp0, temp1, temp2;

	temp = allpassDelays_[0].Get();
	temp0 = allpassCoefficient_ * temp;
	temp0 += input;
	allpassDelays_[0].Add((int) temp0);
	temp0 = -(allpassCoefficient_ * temp0) + temp;

	temp = allpassDelays_[1].Get();
	temp1 = allpassCoefficient_ * temp;
	temp1 += temp0;
	allpassDelays_[1].Add((int) temp1);
	temp1 = -(allpassCoefficient_ * temp1) + temp;

	temp = allpassDelays_[2].Get();
	temp2 = allpassCoefficient_ * temp;
	temp2 += temp1;
	allpassDelays_[2].Add((int) temp2);
	temp2 = -(allpassCoefficient_ * temp2) + temp;

	const double temp3 = temp2 + (combCoefficient_[0] * combDelays_[0].Get());
	const double temp4 = temp2 + (combCoefficient_[1] * combDelays_[1].Get());
	const double temp5 = temp2 + (combCoefficient_[2] * combDelays_[2].Get());
	const double temp6 = temp2 + (combCoefficient_[3] * combDelays_[3].Get());

	combDelays_[0].Add((int) temp3);
	combDelays_[1].Add((int) temp4);
	combDelays_[2].Add((int) temp5);
	combDelays_[3].Add((int) temp6);

	return (temp3 + temp4 + temp5 + temp6) * (double) 0.5;
}


/******************************************************************************\
* GUI utilities                                                                *
\******************************************************************************/
/* About dialog ------------------------------------------------------------- */
CAboutDlg::CAboutDlg(QWidget* parent, const char* name, bool modal, WFlags f)
	: CAboutDlgBase(parent, name, modal, f)
{
	/* Set the text for the about dialog html text control */
	TextViewCredits->setText(
		"<p>" /* General description of llcon software */
		"<big><b>llcon</b> " + tr("Client/Server communication tool to enable "
		"musician to play together through a conventional broadband internet "
		"connection (like DSL).") + "</big>"
		"</p><br>"
		"<p><font face=\"courier\">" /* GPL header text */
		"This program is free software; you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation; either version 2 of the License, or "
		"(at your option) any later version.<br>This program is distributed in "
		"the hope that it will be useful, but WITHOUT ANY WARRANTY; without "
		"even the implied warranty of MERCHANTABILITY or FITNESS FOR A "
		"PARTICULAR PURPOSE. See the GNU General Public License for more "
		"details.<br>You should have received a copy of the GNU General Public "
		"License along with his program; if not, write to the Free Software "
		"Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 "
		"USA"
		"</font></p><br>"
		"<p>" /* Libraries used by this compilation of Dream */
		"<b>" + tr("llcon uses the following libraries or code snippets:") +
		"</b></p>"
		"<ul>"
		"<li>audio reverberation code: by Perry R. Cook and Gary P. Scavone, "
		"1995 - 2004 (taken from \"The Synthesis ToolKit in C++ (STK)\")</li>"
		"<li>IMA-ADPCM: by Erik de Castro Lopo</li>"
		"<li>INI File Tools (STLINI): Robert Kesterson in 1999</li>"
		"<li>Parts from Dream DRM Receiver by Volker Fischer and Alexander "
		"Kurpiers</li>"
		"</ul>"
		"</center><br>");

	/* Set version number in about dialog */
	TextLabelVersion->setText(GetVersionAndNameStr());
}

QString CAboutDlg::GetVersionAndNameStr ( const bool bWithHtml )
{
	QString strVersionText = "";

	// name, short description and GPL hint
	if ( bWithHtml )
	{
		strVersionText += "<center><b>";
	}

	strVersionText += tr("llcon, Version ") + VERSION;

	if ( bWithHtml )
	{
		strVersionText += "</b><br>";
	}
	else
	{
		strVersionText += "\n";
	}

	strVersionText += tr("llcon, Low-Latency (Internet) Connection");

	if ( bWithHtml )
	{
		strVersionText += "<br>";
	}
	else
	{
		strVersionText += "\n";
	}

	strVersionText += tr("Under the GNU General Public License (GPL)");

	if ( bWithHtml )
	{
		strVersionText += "</center>";
	}

	return strVersionText;
}


/* Help menu ---------------------------------------------------------------- */
CLlconHelpMenu::CLlconHelpMenu ( QWidget* parent ) : QPopupMenu ( parent )
{
	/* Standard help menu consists of about and what's this help */
	insertItem ( tr ( "What's &This" ), this ,
		SLOT ( OnHelpWhatsThis () ), SHIFT+Key_F1 );
	insertSeparator();
	insertItem ( tr ( "&About..." ), this, SLOT ( OnHelpAbout () ) );
}
