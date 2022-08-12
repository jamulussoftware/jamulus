/******************************************************************************\
 * Copyright (c) 2004-2022
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

#include "activity.h"
#include <Foundation/Foundation.h>

class CActivityId
{
public:
    id<NSObject> activityId;
};

CActivity::CActivity() : pActivity ( new CActivityId() ) {}

CActivity::~CActivity() { delete pActivity; }

void CActivity::BeginActivity()
{
    NSActivityOptions options =
        NSActivityBackground | NSActivityIdleDisplaySleepDisabled | NSActivityIdleSystemSleepDisabled | NSActivityLatencyCritical;

    pActivity->activityId = [[NSProcessInfo processInfo]
        beginActivityWithOptions:options
                          reason:@"Jamulus provides low latency audio processing and should not be inturrupted by system throttling."];
}

void CActivity::EndActivity()
{
    [[NSProcessInfo processInfo] endActivity:pActivity->activityId];

    pActivity->activityId = nil;
}
