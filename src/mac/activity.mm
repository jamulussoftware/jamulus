/******************************************************************************\
 * Copyright (c) 2004-2026
 *
 * Author(s):
 *  AronVietti
 *
 * As of Jamulus 3.12.1dev (commit eb172d47): All new source code contributions must be licensed
 * under AGPL 3.0 or any later version.
 *
 * Existing code: Code contributed before 3.12.1dev (commit eb172d47) was licensed under GPL 2.0+.
 * This code will be licensed under GPL 3.0 (or any later version) from
 * 3.12.1dev (commit eb172d47).  When distributed as part of Jamulus, the AGPL 3.0 terms govern
 * the combined work, including network use provisions.
 *
 ******************************************************************************
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------------------
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
