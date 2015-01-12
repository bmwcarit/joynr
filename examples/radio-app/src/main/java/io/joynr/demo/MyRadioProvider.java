package io.joynr.demo;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
import io.joynr.exceptions.JoynrArbitrationException;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import joynr.vehicle.Country;
import joynr.vehicle.GeoPosition;
import joynr.vehicle.RadioAbstractProvider;
import joynr.vehicle.RadioStation;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class MyRadioProvider extends RadioAbstractProvider {
    private static final String PRINT_BORDER = "\n####################\n";
    private static final Logger LOG = LoggerFactory.getLogger(MyRadioProvider.class);

    private List<RadioStation> stationsList = new ArrayList<RadioStation>();
    private Map<Country, GeoPosition> countryGeoPositionMap = new HashMap<Country, GeoPosition>();

    private int notSoRandomCounter = 0;

    public MyRadioProvider() {
        providerQos.setPriority(System.currentTimeMillis());
        stationsList.add(new RadioStation("ABC Trible J", false, Country.AUSTRALIA));
        stationsList.add(new RadioStation("Radio Popolare", true, Country.ITALY));
        stationsList.add(new RadioStation("JAZZ.FM91", false, Country.CANADA));
        stationsList.add(new RadioStation("Bayern 3", true, Country.GERMANY));
        countryGeoPositionMap.put(Country.AUSTRALIA, new GeoPosition(-37.8141070, 144.9632800)); // Melbourne
        countryGeoPositionMap.put(Country.ITALY, new GeoPosition(46.4982950, 11.3547580)); // Bolzano
        countryGeoPositionMap.put(Country.CANADA, new GeoPosition(53.5443890, -113.4909270)); // Edmonton
        countryGeoPositionMap.put(Country.GERMANY, new GeoPosition(48.1351250, 11.5819810)); // Munich
        currentStation = stationsList.get(notSoRandomCounter);
    }

    @Override
    public RadioStation getCurrentStation() throws JoynrArbitrationException {
        LOG.debug(PRINT_BORDER + "getCurrentSation -> " + currentStation + PRINT_BORDER);
        return currentStation;
    }

    @Override
    public void shuffleStations() throws JoynrArbitrationException {
        RadioStation oldStation = currentStation;
        notSoRandomCounter++;
        notSoRandomCounter = notSoRandomCounter % stationsList.size();
        currentStationChanged(stationsList.get(notSoRandomCounter));
        LOG.debug(PRINT_BORDER + "shuffleStations: " + oldStation + " -> " + currentStation + PRINT_BORDER);
    }

    @Override
    public Boolean addFavouriteStation(RadioStation radioStation) throws JoynrArbitrationException {
        LOG.debug(PRINT_BORDER + "addFavouriteStation(" + radioStation + ")" + PRINT_BORDER);
        stationsList.add(radioStation);
        return true;
    }

    public void fireWeakSignalEvent() {
        LOG.info(PRINT_BORDER + "fire weakSignalEvent: " + currentStation + PRINT_BORDER);
        weakSignalEventOccurred(currentStation);
    }

    public void fireNewStationDiscoveredEvent() {
        RadioStation discoveredStation = stationsList.get(notSoRandomCounter);
        GeoPosition geoPosition = countryGeoPositionMap.get(discoveredStation.getCountry());
        LOG.info(PRINT_BORDER + "fire newStationDiscoveredEvent: " + discoveredStation + " at " + geoPosition
                + PRINT_BORDER);
        newStationDiscoveredEventOccurred(discoveredStation, geoPosition);
    }
}
