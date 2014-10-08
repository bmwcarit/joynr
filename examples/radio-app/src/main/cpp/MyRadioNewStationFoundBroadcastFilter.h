/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2014 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */
#ifndef MYRADIONEWSTATIONFOUNDFILTER_H
#define MYRADIONEWSTATIONFOUNDFILTER_H

#include "joynr/vehicle/RadioNewStationFoundBroadcastFilter.h"

using namespace joynr;

class MyRadioNewStationFoundBroadcastFilter : public vehicle::RadioNewStationFoundBroadcastFilter
{
public:
    MyRadioNewStationFoundBroadcastFilter();

    virtual bool filter(
            const vehicle::RadioStation &radioStation,
            const bool &hasTrafficInfo,
            const vehicle::RadioNewStationFoundBroadcastFilterParameters &filterParameters);
};

#endif // MYRADIONEWSTATIONFOUNDFILTER_H
