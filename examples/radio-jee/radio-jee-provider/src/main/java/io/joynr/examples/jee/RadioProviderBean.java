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

import java.time.Instant;
import java.util.Set;
import java.util.stream.Collectors;

import javax.ejb.Stateless;
import javax.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.ThreadLocalRandom;

import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import io.joynr.statusmetrics.JoynrStatusMetrics;
import joynr.exceptions.ApplicationException;
import joynr.exceptions.ProviderRuntimeException;
import joynr.vehicle.RadioStation;
import joynr.vehicle.RadioSubscriptionPublisher;
import joynr.vehicle.RadioSync;

/**
 * Sample implementation of the {@link RadioSync} interface.
 */
@Stateless
@ServiceProvider(serviceInterface = RadioSync.class)
public class RadioProviderBean implements RadioService {

    private final static Logger logger = LoggerFactory.getLogger(RadioProviderBean.class);

    private RadioStationDatabase radioStationDatabase;

    private GeoLocationService geoLocationService;

    private RadioSubscriptionPublisher radioSubscriptionPublisher;

    private JoynrStatusMetrics statusMetrics;

    @Inject
    public RadioProviderBean(RadioStationDatabase radioStationDatabase,
                             GeoLocationService geoLocationService,
                             JoynrStatusMetrics statusMetrics,
                             @SubscriptionPublisher RadioSubscriptionPublisher radioSubscriptionPublisher) {
        this.radioStationDatabase = radioStationDatabase;
        this.geoLocationService = geoLocationService;
        this.statusMetrics = statusMetrics;
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
    public Boolean addFavoriteStation(RadioStation newFavoriteStation) throws ProviderRuntimeException,
                                                                       ApplicationException {
        if (newFavoriteStation.getName().isEmpty()) {
            throw new ProviderRuntimeException("MISSING_NAME");
        }
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

    @Override
    public String printMetrics() {
        StringBuilder statusMetricsStringBuilder = new StringBuilder();
        statusMetricsStringBuilder.append("******************************\n");
        statusMetricsStringBuilder.append(" *** Joynr Status Metrics ***\n");
        statusMetricsStringBuilder.append(" *** (");
        statusMetricsStringBuilder.append(Instant.now());
        statusMetricsStringBuilder.append(") ***\n");
        statusMetricsStringBuilder.append("\tDropped messages: " + statusMetrics.getNumDroppedMessages() + "\n");
        statusMetricsStringBuilder.append(statusMetrics.getAllConnectionStatusMetrics().stream().map(m -> {
            String connectionMetricsString = " *** ConnectionStatusMetrics ***\n";
            connectionMetricsString += "\tGBID: >" + m.getGbid().orElse("NULL") + "<\n";
            connectionMetricsString += "\tURL: " + m.getUrl() + "\n";
            connectionMetricsString += "\tisSender?: " + m.isSender() + "\n";
            connectionMetricsString += "\tisReceiver: " + m.isReceiver() + "\n";
            connectionMetricsString += "\tconnection attempts: " + m.getConnectionAttempts() + "\n";
            connectionMetricsString += "\tconnection drops: " + m.getConnectionDrops() + "\n";
            connectionMetricsString += "\tRECEIVED messages: " + m.getReceivedMessages() + "\n";
            connectionMetricsString += "\tSENT messages: " + m.getSentMessages() + "\n";
            connectionMetricsString += "\tlast connection state change: " + m.getLastConnectionStateChangeDate() + "\n";
            connectionMetricsString += "\tisConnected?: " + m.isConnected() + "\n";
            connectionMetricsString += "******************************\n";
            return connectionMetricsString;
        }).collect(Collectors.joining()));
        String statusMetricsString = statusMetricsStringBuilder.toString();
        logger.info(statusMetricsString);
        return statusMetricsString;
    }

}
