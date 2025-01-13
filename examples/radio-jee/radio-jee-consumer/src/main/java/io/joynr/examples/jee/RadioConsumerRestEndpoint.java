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

import java.util.concurrent.CompletableFuture;

import jakarta.inject.Inject;
import jakarta.ws.rs.GET;
import jakarta.ws.rs.POST;
import jakarta.ws.rs.Path;
import jakarta.ws.rs.Produces;
import jakarta.ws.rs.core.MediaType;
import jakarta.ws.rs.core.Response;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.jeeintegration.api.ServiceLocator;
import joynr.exceptions.ApplicationException;
import joynr.vehicle.Country;
import joynr.vehicle.GeoPosition;
import joynr.vehicle.RadioStation;
import joynr.vehicle.RadioSync;

@Path("/radio-stations")
@Produces(MediaType.APPLICATION_JSON)
public class RadioConsumerRestEndpoint {

    private final static Logger logger = LoggerFactory.getLogger(RadioConsumerRestEndpoint.class);

    private ServiceLocator serviceLocator;

    private volatile RadioSync radioClient;

    @Inject
    public RadioConsumerRestEndpoint(ServiceLocator serviceLocator) {
        this.serviceLocator = serviceLocator;
    }

    @GET
    @Path("/current-station")
    public RadioStation getCurrentRadioStation() {
        return getRadioClient().getCurrentStation();
    }

    @GET
    @Path("/current-station/location")
    public GeoPosition getLocationOfCurrentStation() {
        return getRadioClient().getLocationOfCurrentStation().location;
    }

    @GET
    @Path("/current-station/country")
    public Country getCountryOfCurrentStation() {
        return getRadioClient().getLocationOfCurrentStation().country;
    }

    @POST
    @Path("/shuffle")
    public void shuffleRadioStations() {
        getRadioClient().shuffleStations();
    }

    @POST
    @Produces({ MediaType.TEXT_PLAIN })
    public boolean addRadioStation(String name) throws JoynrWebApplicationException {
        try {
            return getRadioClient().addFavoriteStation(new RadioStation(name, true, Country.GERMANY));
        } catch (ApplicationException e) {
            throw new JoynrWebApplicationException(e.getMessage(), Response.Status.INTERNAL_SERVER_ERROR);
        }
    }

    private RadioSync getRadioClient() {
        if (radioClient == null) {
            synchronized (this) {
                if (radioClient == null) {
                    CompletableFuture<RadioSync> future = serviceLocator.builder(RadioSync.class,
                                                                                 "io.joynr.examples.jee.provider")
                                                                        .withDiscoveryQos(new DiscoveryQos(10000L,
                                                                                                           ArbitrationStrategy.HighestPriority,
                                                                                                           360000L))
                                                                        .useFuture()
                                                                        .build();
                    try {
                        radioClient = future.get();
                    } catch (Exception e) {
                        logger.error("Unable to create a proxy for the radio provider. All calls will fail.", e);
                        throw new RuntimeException("Radio proxy creation failed.", e);
                    }
                }
            }
        }
        return radioClient;
    }

}
