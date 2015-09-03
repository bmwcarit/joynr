package io.joynr.demo;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import joynr.vehicle.Country;
import joynr.vehicle.GeoPosition;
import joynr.vehicle.RadioAbstractProvider;
import joynr.vehicle.RadioProvider;
import joynr.vehicle.RadioStation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class MyRadioProvider extends RadioAbstractProvider {
    private static final String PRINT_BORDER = "\n####################\n";
    private static final Logger LOG = LoggerFactory.getLogger(MyRadioProvider.class);

    private RadioStation currentStation;
    private List<RadioStation> stationsList = new ArrayList<RadioStation>();
    private Map<Country, GeoPosition> countryGeoPositionMap = new HashMap<Country, GeoPosition>();

    private int currentStationIndex = 0;

    public MyRadioProvider() {
        providerQos.setPriority(System.currentTimeMillis());
        stationsList.add(new RadioStation("ABC Trible J", true, Country.AUSTRALIA));
        stationsList.add(new RadioStation("Radio Popolare", false, Country.ITALY));
        stationsList.add(new RadioStation("JAZZ.FM91", false, Country.CANADA));
        stationsList.add(new RadioStation("Bayern 3", true, Country.GERMANY));
        countryGeoPositionMap.put(Country.AUSTRALIA, new GeoPosition(-37.8141070, 144.9632800)); // Melbourne
        countryGeoPositionMap.put(Country.ITALY, new GeoPosition(46.4982950, 11.3547580)); // Bolzano
        countryGeoPositionMap.put(Country.CANADA, new GeoPosition(53.5443890, -113.4909270)); // Edmonton
        countryGeoPositionMap.put(Country.GERMANY, new GeoPosition(48.1351250, 11.5819810)); // Munich
        currentStation = stationsList.get(currentStationIndex);
    }

    @Override
    public Promise<Deferred<RadioStation>> getCurrentStation() {
        Deferred<RadioStation> deferred = new Deferred<RadioStation>();
        LOG.info(PRINT_BORDER + "getCurrentSation -> " + currentStation + PRINT_BORDER);
        deferred.resolve(currentStation);
        return new Promise<Deferred<RadioStation>>(deferred);
    }

    @Override
    public Promise<DeferredVoid> shuffleStations() {
        DeferredVoid deferred = new DeferredVoid();
        RadioStation oldStation = currentStation;
        currentStationIndex++;
        currentStationIndex = currentStationIndex % stationsList.size();
        currentStation = stationsList.get(currentStationIndex);
        currentStationChanged(currentStation);
        LOG.info(PRINT_BORDER + "shuffleStations: " + oldStation + " -> " + currentStation + PRINT_BORDER);
        deferred.resolve();
        return new Promise<DeferredVoid>(deferred);
    }

    @Override
    public Promise<AddFavoriteStationDeferred> addFavoriteStation(RadioStation radioStation) {
        AddFavoriteStationDeferred deferred = new AddFavoriteStationDeferred();
        LOG.info(PRINT_BORDER + "addFavoriteStation(" + radioStation + ")" + PRINT_BORDER);
        stationsList.add(radioStation);
        deferred.resolve(true);
        return new Promise<AddFavoriteStationDeferred>(deferred);
    }

    public void fireWeakSignalEvent() {
        LOG.info(PRINT_BORDER + "fire weakSignalEvent: " + currentStation + PRINT_BORDER);
        fireWeakSignal(currentStation);
    }

    public void fireNewStationDiscoveredEvent() {
        RadioStation discoveredStation = currentStation;
        GeoPosition geoPosition = countryGeoPositionMap.get(discoveredStation.getCountry());
        LOG.info(PRINT_BORDER + "fire newStationDiscoveredEvent: " + discoveredStation + " at " + geoPosition
                + PRINT_BORDER);
        fireNewStationDiscovered(discoveredStation, geoPosition);
    }

    @Override
    public Promise<GetLocationOfCurrentStationDeferred> getLocationOfCurrentStation() {
        Country country = currentStation.getCountry();
        GeoPosition location = countryGeoPositionMap.get(country);
        LOG.info(PRINT_BORDER + "getLocationOfCurrentStation: country: " + country.name() + ", location: " + location
                + PRINT_BORDER);
        RadioProvider.GetLocationOfCurrentStationDeferred deferred = new GetLocationOfCurrentStationDeferred();
        deferred.resolve(country, location);
        return new Promise<RadioProvider.GetLocationOfCurrentStationDeferred>(deferred);
    }
}
