package io.joynr.examples.jee;

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

import javax.inject.Inject;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.WebApplicationException;
import javax.ws.rs.core.MediaType;

import io.joynr.jeeintegration.api.ServiceLocator;
import joynr.exceptions.ApplicationException;
import joynr.vehicle.Country;
import joynr.vehicle.GeoPosition;
import joynr.vehicle.RadioStation;
import joynr.vehicle.RadioSync;

@Path("/radio-stations")
@Produces(MediaType.APPLICATION_JSON)
public class RadioConsumerRestEndpoint {

    private ServiceLocator serviceLocator;

    private RadioSync radioClient;

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
    public boolean addRadioStation(RadioStation radioStation) {
        try {
            return getRadioClient().addFavoriteStation(radioStation);
        } catch (ApplicationException e) {
            throw new WebApplicationException(e.getMessage());
        }
    }

    private RadioSync getRadioClient() {
        if (radioClient == null) {
            radioClient = serviceLocator.get(RadioSync.class, "io.joynr.examples.jee.provider");
        }
        return radioClient;
    }

}
