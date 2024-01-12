/******************************************************************************\
 * Copyright (c) 2004-2024
 *
 * Author(s):
 *  AronVietti
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
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
\******************************************************************************/
#pragma once

// Forward Delcaration to pointer that holds the activity id
class CActivityId;

// Represents an OSX specific activity. See Managing Activities
// https://developer.apple.com/documentation/foundation/nsprocessinfo?language=objc
// This essentially lets us start and stop an Activity frame where we tell the OS
// that we are Latency Critical and are not performing background tasks.
class CActivity
{
private:
    CActivityId* pActivity;

public:
    CActivity();

    ~CActivity();

    void BeginActivity();

    void EndActivity();
};
