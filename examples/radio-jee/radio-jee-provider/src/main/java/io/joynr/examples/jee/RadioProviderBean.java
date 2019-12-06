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
package io.joynr.examples.jee;

import java.util.Set;

import javax.ejb.Stateless;
import javax.inject.Inject;

import java.util.concurrent.ThreadLocalRandom;

import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import joynr.exceptions.ApplicationException;
import joynr.vehicle.RadioStation;
import joynr.vehicle.RadioSubscriptionPublisher;
import joynr.vehicle.RadioSync;

/**
 * Sample implementation of the {@link RadioSync} interface.
 */
@Stateless
@ServiceProvider(serviceInterface = RadioSync.class)
public class RadioProviderBean implements RadioService {

    private RadioStationDatabase radioStationDatabase;

    private GeoLocationService geoLocationService;

    private RadioSubscriptionPublisher radioSubscriptionPublisher;

    @Inject
    public RadioProviderBean(RadioStationDatabase radioStationDatabase,
                             GeoLocationService geoLocationService,
                             @SubscriptionPublisher RadioSubscriptionPublisher radioSubscriptionPublisher) {
        this.radioStationDatabase = radioStationDatabase;
        this.geoLocationService = geoLocationService;
        this.radioSubscriptionPublisher = radioSubscriptionPublisher;
    }

    @Override
    public RadioStation getCurrentStation() {
        return radioStationDatabase.getCurrentStation();
    }

    @Override
    public void shuffleStations() {
        Set<RadioStation> radioStations = radioStationDatabase.getRadioStations();
        int randomIndex = ThreadLocalRandom.current().nextInt(radioStations.size());
        radioStationDatabase.setCurrentStation(radioStations.toArray(new RadioStation[0])[randomIndex]);
    }

    @Override
    public Boolean addFavoriteStation(RadioStation newFavoriteStation) throws ApplicationException {
        radioStationDatabase.addRadioStation(newFavoriteStation);
        return true;
    }

    @Override
    public GetLocationOfCurrentStationReturned getLocationOfCurrentStation() {
        RadioStation currentStation = radioStationDatabase.getCurrentStation();
        GetLocationOfCurrentStationReturned result = null;
        if (currentStation != null) {
            result = new GetLocationOfCurrentStationReturned(currentStation.getCountry(),
                                                             geoLocationService.getPositionFor(currentStation.getCountry()));
        }
        return result;
    }

    @Override
    public void fireWeakSignal() {
        if (radioSubscriptionPublisher == null) {
            throw new IllegalStateException("No subscription publisher available.");
        }
        radioSubscriptionPublisher.fireWeakSignal(radioStationDatabase.getCurrentStation());
    }

}
