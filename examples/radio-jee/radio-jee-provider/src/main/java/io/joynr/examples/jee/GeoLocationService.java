/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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
package io.joynr.examples.jee;

import java.util.HashMap;
import java.util.Map;

import jakarta.annotation.PostConstruct;
import jakarta.ejb.Singleton;

import joynr.vehicle.Country;
import joynr.vehicle.GeoPosition;

/**
 * A dummy implementation of a geo-location service which returns lat/long values given a {@link Country}.
 */
@Singleton
public class GeoLocationService {

    private Map<Country, GeoPosition> geoPositions = new HashMap<>();

    @PostConstruct
    public void initialiseGeoPositions() {
        geoPositions.put(Country.AUSTRALIA, new GeoPosition(-37.8141070, 144.9632800)); // Melbourne
        geoPositions.put(Country.ITALY, new GeoPosition(46.4982950, 11.3547580)); // Bolzano
        geoPositions.put(Country.CANADA, new GeoPosition(53.5443890, -113.4909270)); // Edmonton
        geoPositions.put(Country.GERMANY, new GeoPosition(48.1351250, 11.5819810)); // Munich
    }

    public GeoPosition getPositionFor(Country country) {
        return geoPositions.get(country);
    }

}
