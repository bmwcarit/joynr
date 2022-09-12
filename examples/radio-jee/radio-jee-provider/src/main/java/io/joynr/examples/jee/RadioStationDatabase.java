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

import java.util.Collections;
import java.util.HashSet;
import java.util.Set;

import javax.annotation.PostConstruct;
import javax.ejb.Singleton;

import joynr.exceptions.ApplicationException;
import joynr.vehicle.Country;
import joynr.vehicle.Radio;
import joynr.vehicle.RadioStation;

/**
 * A very simple in-memory radio station database.
 */
@Singleton
public class RadioStationDatabase {

    private Set<RadioStation> radioStations = new HashSet<>();

    private RadioStation currentStation;

    @PostConstruct
    public void initialiseRadioStations() {
        radioStations.add(new RadioStation("ABC Trible J", true, Country.AUSTRALIA));
        radioStations.add(new RadioStation("Radio Popolare", false, Country.ITALY));
        radioStations.add(new RadioStation("JAZZ.FM91", false, Country.CANADA));
        radioStations.add(new RadioStation("Bayern 3", true, Country.GERMANY));
        currentStation = radioStations.iterator().next();
    }

    public RadioStation getCurrentStation() {
        return currentStation;
    }

    public void setCurrentStation(RadioStation currentStation) {
        if (!radioStations.contains(currentStation)) {
            radioStations.add(currentStation);
        }
        this.currentStation = currentStation;
    }

    public void addRadioStation(RadioStation newRadioStation) throws ApplicationException {
        if (radioStations.contains(newRadioStation)) {
            throw new ApplicationException(Radio.AddFavoriteStationErrorEnum.DUPLICATE_RADIOSTATION);
        }
        radioStations.add(newRadioStation);
    }

    public Set<RadioStation> getRadioStations() {
        return Collections.unmodifiableSet(radioStations);
    }

}
