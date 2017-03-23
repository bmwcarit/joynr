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
#include "GeocastBroadcastFilter.h"

#include <string>

#include <boost/geometry.hpp>

#include "joynr/serializer/Serializer.h"

INIT_LOGGER(GeocastBroadcastFilter);

GeocastBroadcastFilter::GeocastBroadcastFilter()
{
}

bool GeocastBroadcastFilter::filter(
        const joynr::vehicle::RadioStation& discoveredStation,
        const joynr::vehicle::GeoPosition& geoPosition,
        const vehicle::RadioNewStationDiscoveredBroadcastFilterParameters& filterParameters)
{
    if (filterParameters.getPositionOfInterest().empty() ||
        filterParameters.getRadiusOfInterestArea().empty()) {
        // filter parameter not set, so we do no filtering
        return true;
    }

    joynr::vehicle::GeoPosition positionOfInterest;
    try {
        joynr::serializer::deserializeFromJson(
                positionOfInterest, filterParameters.getPositionOfInterest());
    } catch (const std::invalid_argument& e) {
        JOYNR_LOG_ERROR(logger,
                        "Unable to deserialize geo position object from: {} - error: {}",
                        filterParameters.getPositionOfInterest(),
                        e.what());
        return true;
    }
    int radiusOfInterestArea = std::stoi(filterParameters.getRadiusOfInterestArea());

    // calculate distance between two geo positions using the haversine formula
    // (cf. http://en.wikipedia.org/wiki/Haversine_formula)
    double earthRadius = 6371000.0; // in meters

    typedef boost::geometry::model::point<
            double,
            2,
            boost::geometry::cs::spherical_equatorial<boost::geometry::degree>> BoostGeoPosition;

    BoostGeoPosition boostGeoPosition(geoPosition.getLatitude(), geoPosition.getLongitude());
    BoostGeoPosition boostPositionOfInterest(
            positionOfInterest.getLatitude(), positionOfInterest.getLongitude());

    double distance = boost::geometry::distance(
            boostGeoPosition,
            boostPositionOfInterest,
            boost::geometry::strategy::distance::haversine<double>(earthRadius));

    return distance < radiusOfInterestArea;
}
