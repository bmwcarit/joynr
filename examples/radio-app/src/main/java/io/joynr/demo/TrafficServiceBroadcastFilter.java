package io.joynr.demo;

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

import joynr.vehicle.GeoPosition;
import joynr.vehicle.RadioBroadcastInterface.NewStationDiscoveredBroadcastFilterParameters;
import joynr.vehicle.RadioNewStationDiscoveredBroadcastFilter;
import joynr.vehicle.RadioStation;

public class TrafficServiceBroadcastFilter extends RadioNewStationDiscoveredBroadcastFilter {

    @Override
    public boolean filter(RadioStation discoveredStation,
                          GeoPosition geoPosition,
                          NewStationDiscoveredBroadcastFilterParameters filterParameters) {
        if (filterParameters.getHasTrafficService() == null) {
            // filter parameter not set, so we do no filtering
            return true;
        }
        Boolean hasTrafficService = Boolean.valueOf(filterParameters.getHasTrafficService());
        return discoveredStation.getTrafficService().equals(hasTrafficService);
    }

}
