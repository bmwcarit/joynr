/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#ifndef GEOCASTBROADCASTFILTER_H
#define GEOCASTBROADCASTFILTER_H

#include "joynr/vehicle/RadioNewStationDiscoveredBroadcastFilter.h"

#include "joynr/Logger.h"

using namespace joynr;

class GeocastBroadcastFilter : public vehicle::RadioNewStationDiscoveredBroadcastFilter
{
public:
    GeocastBroadcastFilter();

    virtual bool filter(
            const joynr::vehicle::RadioStation& discoveredStation,
            const joynr::vehicle::GeoPosition& geoPosition,
            const vehicle::RadioNewStationDiscoveredBroadcastFilterParameters& filterParameters);

private:
    ADD_LOGGER(GeocastBroadcastFilter);
};

#endif // GEOCASTBROADCASTFILTER_H
