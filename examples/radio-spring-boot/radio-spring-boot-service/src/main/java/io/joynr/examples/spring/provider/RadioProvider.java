/*
 * #%L
 * %%
 * Copyright (C) 2023 BMW Car IT GmbH
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
package io.joynr.examples.spring.provider;

import com.google.inject.Injector;
import io.joynr.messaging.JoynrMessageMetaInfo;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;
import io.joynr.provider.Promise;
import joynr.exceptions.ProviderRuntimeException;
import joynr.vehicle.Country;
import joynr.vehicle.GeoPosition;
import joynr.vehicle.Radio;
import joynr.vehicle.RadioAbstractProvider;
import joynr.vehicle.RadioStation;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.ThreadLocalRandom;

public class RadioProvider extends RadioAbstractProvider {

    private static final Logger logger = LoggerFactory.getLogger(RadioProvider.class);
    private static final String PRINT_BORDER = "\n####################\n";
    public static final String MISSING_NAME = "MISSING_NAME";
    private static final String CONTEXT_KEY = "someKey";

    private final List<RadioStation> radioStations = new ArrayList<>();
    private final Map<Country, GeoPosition> geoPositions = new HashMap<>();
    private RadioStation currentStation;

    private int getCurrentStationInvocationCount = 0;
    private int shuffleStationsInvocationCount = 0;

    private final Injector injector;

    public RadioProvider(final Injector injector) {
        this.injector = injector;
        init();
    }

    private void init() {
        radioStations.clear();
        radioStations.add(new RadioStation("ABC Trible J", true, Country.AUSTRALIA));
        radioStations.add(new RadioStation("Radio Popolare", false, Country.ITALY));
        radioStations.add(new RadioStation("JAZZ.FM91", false, Country.CANADA));
        radioStations.add(new RadioStation("Bayern 3", true, Country.GERMANY));
        geoPositions.clear();
        geoPositions.put(Country.AUSTRALIA, new GeoPosition(-37.8141070, 144.9632800)); // Melbourne
        geoPositions.put(Country.ITALY, new GeoPosition(46.4982950, 11.3547580)); // Bolzano
        geoPositions.put(Country.CANADA, new GeoPosition(53.5443890, -113.4909270)); // Edmonton
        geoPositions.put(Country.GERMANY, new GeoPosition(48.1351250, 11.5819810)); // Munich
        currentStation = radioStations.iterator().next();
    }

    @Override
    public Promise<Deferred<RadioStation>> getCurrentStation() {
        final Deferred<RadioStation> deferred = new Deferred<>();
        logger.info(PRINT_BORDER + "getCurrentStation -> {}" + PRINT_BORDER, this.currentStation);
        deferred.resolve(currentStation);
        getCurrentStationInvocationCount++;
        return new Promise<>(deferred);
    }

    @Override
    public Promise<DeferredVoid> shuffleStations() {
        final Serializable initialValue = getMessageContextValue(CONTEXT_KEY).orElse("default-value");
        setMessageContextValue(CONTEXT_KEY, initialValue);
        final Serializable modifiedValue = "non-" + getMessageContextValue(CONTEXT_KEY).orElse("this-should-not-be-empty-anymore");
        setMessageContextValue(CONTEXT_KEY, modifiedValue);

        final DeferredVoid deferred = new DeferredVoid();
        this.currentStation = getRandomStation();
        deferred.resolve();
        shuffleStationsInvocationCount++;
        return new Promise<>(deferred);
    }

    @SuppressWarnings("SameParameterValue")
    private void setMessageContextValue(final String key, final Serializable value) {
        final JoynrMessageMetaInfo metaInfo = injector.getInstance(JoynrMessageMetaInfo.class);
        final Map<String, Serializable> context = metaInfo.getMessageContext();
        context.put(key, value);
        metaInfo.setMessageContext(context);
        logger.info("Setting message context key {} with a value {}.", key, value);
    }

    @SuppressWarnings("SameParameterValue")
    private Optional<Serializable> getMessageContextValue(final String key) {
        logger.info("Retrieving vale from message context by key {}", key);
        final JoynrMessageMetaInfo metaInfo = injector.getInstance(JoynrMessageMetaInfo.class);
        final Map<String, Serializable> context = metaInfo.getMessageContext();
        if (context.containsKey(key)) {
            final Serializable value = context.get(key);
            logger.info("Message context contains key {} with a value {}.", key, value);
            return Optional.of(value);
        } else {
            logger.info("Message context does not contain key {}.", key);
            return Optional.empty();
        }
    }

    private RadioStation getRandomStation() {
        final int index = ThreadLocalRandom.current().nextInt(radioStations.size());
        return radioStations.get(index);
    }

    @Override
    public Promise<AddFavoriteStationDeferred> addFavoriteStation(final RadioStation newFavoriteStation) {
        final AddFavoriteStationDeferred deferred = new AddFavoriteStationDeferred();

        if (newFavoriteStation.getName().isEmpty()) {
            deferred.reject(new ProviderRuntimeException(MISSING_NAME));
        }

        boolean duplicateFound = false;
        for (final RadioStation station : this.radioStations) {
            if (station.getName().equals(newFavoriteStation.getName())) {
                duplicateFound = true;
                deferred.reject(Radio.AddFavoriteStationErrorEnum.DUPLICATE_RADIOSTATION);
                break;
            }
        }
        if (!duplicateFound) {
            logger.info(PRINT_BORDER + "addFavoriteStation({})" + PRINT_BORDER, newFavoriteStation);
            this.radioStations.add(newFavoriteStation);
            deferred.resolve(true);
        }

        // Promise is returned immediately. Deferred is resolved later
        return new Promise<>(deferred);
    }

    @Override
    public Promise<GetLocationOfCurrentStationDeferred> getLocationOfCurrentStation() {
        final Country country = currentStation.getCountry();
        final GeoPosition location = this.geoPositions.get(country);
        logger.info(PRINT_BORDER + "getLocationOfCurrentStation: country: {}, location: {}" + PRINT_BORDER,
                    country.name(),
                    location);
        final GetLocationOfCurrentStationDeferred deferred = new GetLocationOfCurrentStationDeferred();
        // actions that take no time can be returned immediately by resolving the deferred.
        deferred.resolve(country, location);
        return new Promise<>(deferred);
    }

    public int getGetCurrentStationInvocationCount() {
        return this.getCurrentStationInvocationCount;
    }

    public int getShuffleStationsInvocationCount() {
        return this.shuffleStationsInvocationCount;
    }
}
