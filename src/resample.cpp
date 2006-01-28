/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 * Resample routine for arbitrary sample-rate conversions in a low range (for
 * frequency offset correction).
 * The algorithm is based on a polyphase structure. We upsample the input
 * signal with a factor INTERP_DECIM_I_D and calculate two successive samples
 * whereby we perform a linear interpolation between these two samples to get
 * an arbitraty sample grid.
 * The polyphase filter is calculated with Matlab(TM), the associated file
 * is ResampleFilter.m.
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

#include "resample.h"


/* Implementation *************************************************************/
int CResample::Resample(CVector<double>& vecdInput, CVector<double>& vecdOutput,
						const double dRation)
{
	int i;

	/* move old data from the end to the history part of the buffer and
	   add new data (shift register) */
	/* Shift old values */
	int iMovLen = iInputBlockSize;
	for (i = 0; i < iHistorySize; i++)
	{
		vecdIntBuff[i] = vecdIntBuff[iMovLen++];
	}

	/* Add new block of data */
	int iBlockEnd = iHistorySize;
	for (i = 0; i < iInputBlockSize; i++)
	{
		vecdIntBuff[iBlockEnd++] = vecdInput[i];
	}

	/* sample-interval of new sample frequency in relation to interpolated
	   sample-interval */
	dTStep = (double) INTERP_DECIM_I_D / dRation;

	/* init output counter */
	int im = 0;

	/* main loop */
	do
	{
		/* quantize output-time to interpolated time-index */
		const int ik = (int) dtOut;


		/* calculate convolutions for the two interpolation-taps ------------ */
		/* phase for the linear interpolation-taps */
		const int ip1 = ik % INTERP_DECIM_I_D;
		const int ip2 = (ik + 1) % INTERP_DECIM_I_D;

		/* sample positions in input vector */
		const int in1 = (int) (ik / INTERP_DECIM_I_D);
		const int in2 = (int) ((ik + 1) / INTERP_DECIM_I_D);

		/* convolution */
		double dy1 = 0.0;
		double dy2 = 0.0;
		for (int i = 0; i < NUM_TAPS_PER_PHASE; i++)
		{
			dy1 += fResTaps1To1[ip1][i] * vecdIntBuff[in1 - i];
			dy2 += fResTaps1To1[ip2][i] * vecdIntBuff[in2 - i];
		}


		/* linear interpolation --------------------------------------------- */
		/* get numbers after the comma */
		const double dxInt = dtOut - (int) dtOut;
		vecdOutput[im] = (dy2 - dy1) * dxInt + dy1;


		/* increase output counter */
		im++;
			
		/* increase output-time and index one step */
		dtOut = dtOut + dTStep;
	} 
	while (dtOut < dBlockDuration);

	/* set rtOut back */
	dtOut -= iInputBlockSize * INTERP_DECIM_I_D;

	return im;
}

void CResample::Init(const int iNewInputBlockSize)
{
	iInputBlockSize = iNewInputBlockSize;

	/* history size must be one sample larger, because we use always TWO
	   convolutions */
	iHistorySize = NUM_TAPS_PER_PHASE + 1;

	/* calculate block duration */
	dBlockDuration = (iInputBlockSize + iHistorySize - 1) * INTERP_DECIM_I_D;

	/* allocate memory for internal buffer, clear sample history */
	vecdIntBuff.Init(iInputBlockSize + iHistorySize, 0.0);

	/* init absolute time for output stream (at the end of the history part */
	dtOut = (double) (iHistorySize - 1) * INTERP_DECIM_I_D;
}

void CAudioResample::Resample(CVector<double>& vecdInput,
							  CVector<double>& vecdOutput)
{
	int j;

	if (dRation == 1.0)
	{
		/* if ratio is 1, no resampling is needed, just copy vector */
		for (j = 0; j < iOutputBlockSize; j++)
			vecdOutput[j] = vecdInput[j];
	}
	else
	{
		/* move old data from the end to the history part of the buffer and
		   add new data (shift register) */
		/* Shift old values */
		int iMovLen = iInputBlockSize;
		for (j = 0; j < NUM_TAPS_PER_PHASE; j++)
			vecdIntBuff[j] = vecdIntBuff[iMovLen++];

		/* Add new block of data */
		int iBlockEnd = NUM_TAPS_PER_PHASE;
		for (j = 0; j < iInputBlockSize; j++)
			vecdIntBuff[iBlockEnd++] = vecdInput[j];

		/* main loop */
		for (j = 0; j < iOutputBlockSize; j++)
		{
			/* phase for the linear interpolation-taps */
			const int ip =
				(int) (j * INTERP_DECIM_I_D / dRation) % INTERP_DECIM_I_D;

			/* sample position in input vector */
			const int in = (int) (j / dRation) + NUM_TAPS_PER_PHASE;

			/* convolution */
			double dy = 0.0;
			for (int i = 0; i < NUM_TAPS_PER_PHASE; i++)
				dy += fResTaps1To1[ip][i] * vecdIntBuff[in - i];

			vecdOutput[j] = dy;
		}
	}
}

void CAudioResample::Init(const int iNewInputBlockSize, const double dNewRation)
{
	dRation = dNewRation;
	iInputBlockSize = iNewInputBlockSize;
	iOutputBlockSize = (int) (iInputBlockSize * dNewRation);

	/* allocate memory for internal buffer, clear sample history */
	vecdIntBuff.Init(iInputBlockSize + NUM_TAPS_PER_PHASE, 0.0);
}
