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

import java.io.IOException;

import joynr.vehicle.GeoPosition;
import joynr.vehicle.RadioBroadcastInterface.NewStationDiscoveredBroadcastFilterParameters;
import joynr.vehicle.RadioNewStationDiscoveredBroadcastFilter;
import joynr.vehicle.RadioStation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.ObjectMapper;

public class GeocastBroadcastFilter extends RadioNewStationDiscoveredBroadcastFilter {
    private static final Logger LOG = LoggerFactory.getLogger(GeocastBroadcastFilter.class);

    private ObjectMapper jsonSerializer;

    public GeocastBroadcastFilter(ObjectMapper jsonSerializer) {
        this.jsonSerializer = jsonSerializer;
    }

    @Override
    public boolean filter(RadioStation discoveredStation,
                          GeoPosition geoPosition,
                          NewStationDiscoveredBroadcastFilterParameters filterParameters) {
        if (filterParameters.getPositionOfInterest() == null || filterParameters.getRadiusOfInterestArea() == null) {
            // filter parameter not set, so we do no filtering
            return true;
        }

        GeoPosition positionOfInterest = null;
        try {
            positionOfInterest = jsonSerializer.readValue(filterParameters.getPositionOfInterest(), GeoPosition.class);
        } catch (IOException e) {
            LOG.error("Unable to parse position of interest from filter parameters. DISABLING filter.", e);
            return true;
        }
        int radiusOfInterestArea = 0;
        try {
            radiusOfInterestArea = Integer.parseInt(filterParameters.getRadiusOfInterestArea());
        } catch (NumberFormatException e) {
            LOG.error("Unable to parse radius of interest area from filter parameters. DISABLING filter.", e);
            return true;
        }

        // calculate distance between two geo positions using the haversine formula
        // (cf. http://en.wikipedia.org/wiki/Haversine_formula)
        int earthRadius = 6371000; // in meters
        double lat1 = Math.toRadians(geoPosition.getLatitude());
        double lat2 = Math.toRadians(positionOfInterest.getLatitude());
        double long1 = Math.toRadians(geoPosition.getLongitude());
        double long2 = Math.toRadians(positionOfInterest.getLongitude());

        double latSinePow = Math.pow(Math.sin((lat2 - lat1) / 2), 2.0);
        double longSinePow = Math.pow(Math.sin((long2 - long1) / 2), 2.0);
        double help = Math.sqrt(latSinePow + Math.cos(lat1) * Math.cos(lat2) * longSinePow);
        // check for floating point errors
        if (help > 1.0) {
            help = 1.0;
        }
        double distance = 2 * earthRadius * Math.asin(help);

        return distance < radiusOfInterestArea;
    }

}
