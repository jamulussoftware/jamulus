/******************************************************************************\
 * Copyright (c) 2004-2006
 *
 * Author(s):
 *	Volker Fischer
 *
 * Note: we assuming here that put and get operations are secured by a mutex
 *       and do not take place at the same time
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

#include "buffer.h"


/* Implementation *************************************************************/
void CNetBuf::Init ( const int iNewBlockSize, const int iNewNumBlocks )
{
	/* total size -> size of one block times number of blocks */
	iBlockSize = iNewBlockSize;
	iMemSize = iNewBlockSize * iNewNumBlocks;

	/* fade in first block added to the buffer */
	bFadeInNewPutData = true;

	/* allocate and clear memory for actual data buffer */
	vecdMemory.Init(iMemSize);

	/* use the "get" flag to make sure the buffer is cleared */
	Clear(CT_GET);

	/* initialize number of samples for fading effect */
	if (FADE_IN_OUT_NUM_SAM < iBlockSize)
		iNumSamFading = iBlockSize;
	else
		iNumSamFading = FADE_IN_OUT_NUM_SAM;

	if (FADE_IN_OUT_NUM_SAM_EXTRA > iBlockSize)
		iNumSamFadingExtra = iBlockSize;
	else
		iNumSamFadingExtra = FADE_IN_OUT_NUM_SAM;

	/* init variables for extrapolation (in case a fade out is needed) */
	dExPDiff = 0.0;
	dExPLastV = 0.0;
}

bool CNetBuf::Put(CVector<double>& vecdData)
{
#ifdef _DEBUG_
static FILE* pFileBI = fopen("bufferin.dat", "w");
fprintf(pFileBI, "%d %d\n", GetAvailSpace() / iBlockSize, iMemSize / iBlockSize);
fflush(pFileBI);
#endif

	bool bPutOK = true;

	/* get size of data to be added to the buffer */
	const int iInSize = vecdData.Size();

	/* Check if there is not enough space available -> correct */
	if (GetAvailSpace() < iInSize)
	{
		/* not enough space in buffer for put operation, correct buffer to
		   prepare for new data */
		Clear(CT_PUT);

		/* set flag to fade in new block to avoid clicks */
		bFadeInNewPutData = true;

		bPutOK = false; /* return error flag */
	}

	/* fade in new block if required */
	if (bFadeInNewPutData)
		FadeInAudioDataBlock(vecdData);

	/* copy new data in internal buffer */
	int iCurPos = 0;
	if (iPutPos + iInSize > iMemSize)
	{
		/* remaining space size for second block */
		const int iRemSpace = iPutPos + iInSize - iMemSize;

		/* data must be written in two steps because of wrap around */
		while (iPutPos < iMemSize)
			vecdMemory[iPutPos++] = vecdData[iCurPos++];

		for (iPutPos = 0; iPutPos < iRemSpace; iPutPos++)
			vecdMemory[iPutPos] = vecdData[iCurPos++];
	}
	else
	{
		/* data can be written in one step */
		const int iEnd = iPutPos + iInSize;
		while (iPutPos < iEnd)
			vecdMemory[iPutPos++] = vecdData[iCurPos++];
	}

	/* set buffer state flag */
	if (iPutPos == iGetPos)
		eBufState = CNetBuf::BS_FULL;
	else
		eBufState = CNetBuf::BS_OK;

	return bPutOK;
}

bool CNetBuf::Get(CVector<double>& vecdData)
{
	bool bGetOK = true; /* init return value */
	bool bFadeOutExtrap = false;

	/* get size of data to be get from the buffer */
	const int iInSize = vecdData.Size();

	/* Check if there is not enough data available -> correct */
	if (GetAvailData() < iInSize)
	{
		/* not enough data in buffer for get operation, correct buffer to
		   prepare for getting data */
		Clear(CT_GET);

		/* set flag to fade in next new block in buffer and fade out last
		   block by extrapolation to avoid clicks */
		bFadeInNewPutData = true;
		bFadeOutExtrap = true;

		bGetOK = false; /* return error flag */
	}

	/* copy data from internal buffer in output buffer */
	int iCurPos = 0;
	if (iGetPos + iInSize > iMemSize)
	{
		/* remaining data size for second block */
		const int iRemData = iGetPos + iInSize - iMemSize;

		/* data must be read in two steps because of wrap around */
		while (iGetPos < iMemSize)
			vecdData[iCurPos++] = vecdMemory[iGetPos++];

		for (iGetPos = 0; iGetPos < iRemData; iGetPos++)
			vecdData[iCurPos++] = vecdMemory[iGetPos];
	}
	else
	{
		/* data can be read in one step */
		const int iEnd = iGetPos + iInSize;
		while (iGetPos < iEnd)
			vecdData[iCurPos++] = vecdMemory[iGetPos++];
	}

	/* set buffer state flag */
	if (iPutPos == iGetPos)
		eBufState = CNetBuf::BS_EMPTY;
	else
		eBufState = CNetBuf::BS_OK;


	/* extrapolate data from old block to avoid "clicks"
	   we have to do this method since we cannot fade out the old block
	   anymore since it is already gone (processed or send through the
	   network) */
	if (bFadeOutExtrap)
		FadeOutExtrapolateAudioDataBlock(vecdData, dExPDiff, dExPLastV);

	/* save some paramters from last block which is needed in case we do not
	   have enough data for next "get" operation and need to extrapolate the
	   signal to avoid "clicks"
	   we assume here that "iBlockSize" is larger than 1! */
	dExPDiff = vecdData[iInSize - 1] - vecdData[iInSize - 2];
	dExPLastV = vecdData[iInSize - 1];

	return bGetOK;
}

int CNetBuf::GetAvailSpace() const
{
	/* calculate available space in buffer */
	int iAvSpace = iGetPos - iPutPos;

	/* check for special case and wrap around */
	if (iAvSpace < 0)
		iAvSpace += iMemSize; /* wrap around */
	else if ((iAvSpace == 0) && (eBufState == BS_EMPTY))
		iAvSpace = iMemSize;

	return iAvSpace;
}

int CNetBuf::GetAvailData() const
{
	/* calculate available data in buffer */
	int iAvData = iPutPos - iGetPos;

	/* check for special case and wrap around */
	if (iAvData < 0)
		iAvData += iMemSize; /* wrap around */
	else if ((iAvData == 0) && (eBufState == BS_FULL))
		iAvData = iMemSize;

	return iAvData;
}

void CNetBuf::Clear(const EClearType eClearType)
{

	int iMiddleOfBuffer;

#if 0
	/* with the following operation we set the new get pos to a block
	   boundary (one block below the middle of the buffer in case of odd
	   number of blocks, e.g.:
	   [buffer size]: [get pos]
	   1: 0   /   2: 0   /   3: 1   /   4: 1   /   ... */
	iMiddleOfBuffer = (((iMemSize - iBlockSize) / 2) / iBlockSize) * iBlockSize;
#else
// old code

// somehow the old code seems to work better than the sophisticated new one....?
	/* 1: 0   /   2: 1   /   3: 1   /   4: 2   /   ... */
	iMiddleOfBuffer = ((iMemSize / 2) / iBlockSize) * iBlockSize;
#endif
	
	
	/* different behaviour for get and put corrections */
	if (eClearType == CT_GET)
	{
		/* clear buffer */
		vecdMemory.Reset(0.0);

		/* correct buffer so that after the current get operation the pointer
		   are at maximum distance */
		iPutPos = 0;
		iGetPos = iMiddleOfBuffer;

		/* check for special case */
		if (iPutPos == iGetPos)
			eBufState = CNetBuf::BS_FULL;
		else
			eBufState = CNetBuf::BS_OK;
	}
	else
	{
		/* in case of "put" correction, do not delete old data but only shift
		   the pointers */
		iPutPos = iMiddleOfBuffer;

		/* adjust put pointer relative to current get pointer, take care of
		   wrap around */
		iPutPos += iGetPos;
		if (iPutPos > iMemSize)
			iPutPos -= iMemSize;

		/* fade out old data right before new put pointer */
		int iCurPos = iPutPos - iNumSamFading;
		int i = iNumSamFading;

		if (iCurPos < 0)
		{
			/* wrap around */
			iCurPos += iMemSize;

			/* data must be processed in two steps because of wrap around */
			while (iCurPos < iMemSize)
			{
				vecdMemory[iCurPos++] *= ((double) i / iNumSamFading);
				i--;
			}

			for (iCurPos = 0; iCurPos < iPutPos; iCurPos++)
			{
				vecdMemory[iCurPos] *= ((double) i / iNumSamFading);
				i--;
			}
		}
		else
		{
			/* data can be processed in one step */
			while (iCurPos < iPutPos)
			{
				vecdMemory[iCurPos++] *= ((double) i / iNumSamFading);
				i--;
			}
		}

		/* check for special case */
		if (iPutPos == iGetPos)
		{
			eBufState = CNetBuf::BS_EMPTY;
		}
		else
		{
			eBufState = CNetBuf::BS_OK;
		}
	}
}

void CNetBuf::FadeInAudioDataBlock(CVector<double>& vecdData)
{
	/* apply linear fading */
	for (int i = 0; i < iNumSamFading; i++)
	{
		vecdData[i] *= ((double) i / iNumSamFading);
	}

	/* reset flag */
	bFadeInNewPutData = false;
}

void CNetBuf::FadeOutExtrapolateAudioDataBlock(CVector<double>& vecdData,
											   const double dExPDiff,
											   const double dExPLastV)
{
	/* apply linear extrapolation and linear fading */
	for (int i = 0; i < iNumSamFadingExtra; i++)
	{
		/* calculate extrapolated value */
		vecdData[i] = ((i + 1) * dExPDiff + dExPLastV);

		/* linear fading */
		vecdData[i] *= ((double) (iNumSamFadingExtra - i) / iNumSamFadingExtra);
	}
}



/* conversion buffer implementation *******************************************/
void CConvBuf::Init ( const int iNewMemSize )
{
	/* set memory size */
	iMemSize = iNewMemSize;

	/* allocate and clear memory for actual data buffer */
	vecsMemory.Init(iMemSize);

	iPutPos = 0;
}

bool CConvBuf::Put ( const CVector<short>& vecsData)
{
	const int iVecSize = vecsData.Size();

	/* copy new data in internal buffer */
	int iCurPos = 0;
	const int iEnd = iPutPos + iVecSize;
	while ( iPutPos < iEnd )
	{
		vecsMemory[iPutPos++] = vecsData[iCurPos++];
	}

	bool bBufOk = false;
	if ( iEnd == iMemSize )
	{
		bBufOk = true;
	}

	return bBufOk;
}

CVector<short> CConvBuf::Get()
{
	iPutPos = 0;
	return vecsMemory;
}
