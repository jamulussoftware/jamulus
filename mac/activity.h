/******************************************************************************\
 * Copyright (c) 2004-2020
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
#pragma once

// Forward Delcaration to pointer that holds the activity id
class CActivityId;

// Reperesents an OSX specific activity. See Managing Activities
// https://developer.apple.com/documentation/foundation/nsprocessinfo?language=objc
// This essentially lets us start and stop an Activity frame where we tell the OS
// that we are Latency Critical and are not performing background tasks.
class CActivity
{
private:
    CActivityId *pActivity;
    
public:
    CActivity();
    
    ~CActivity();
    
    void BeginActivity();

    void EndActivity();
};
