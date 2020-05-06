/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
package io.joynr.examples.statelessasync;

import java.util.List;
import java.util.stream.Collectors;

import javax.inject.Inject;
import javax.ws.rs.GET;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.container.AsyncResponse;
import javax.ws.rs.container.Suspended;
import javax.ws.rs.core.MediaType;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import joynr.examples.statelessasync.KeyValue;
import joynr.examples.statelessasync.VehicleConfiguration;

@Path("/control")
@Produces(MediaType.APPLICATION_JSON)
public class RestEndpoint {

    private static final Logger logger = LoggerFactory.getLogger(RestEndpoint.class);

    @Inject
    private DataAccess dataAccess;

    @Inject
    private WaitForGetResultBean waitForGetResultBean;

    @Inject
    private VehicleStateClientBean vehicleStateClientBean;

    @GET
    @Path("/ping")
    public String ping() {
        return "pong";
    }

    @PUT
    @Path("/vehicleconfig")
    public void addConfiguration(VehicleConfigurationTO data) {
        VehicleConfiguration vehicleConfiguration = new VehicleConfiguration();
        vehicleConfiguration.setId(data.getId());
        vehicleConfiguration.setEntries(data.getEntries()
                                            .entrySet()
                                            .stream()
                                            .map(dataEntry -> new KeyValue(dataEntry.getKey(), dataEntry.getValue()))
                                            .toArray(KeyValue[]::new));
        vehicleStateClientBean.getService()
                              .addConfiguration(vehicleConfiguration,
                                                messageId -> dataAccess.addKnownConfiguration(messageId,
                                                                                              vehicleConfiguration.getId()));
    }

    @GET
    @Path("/vehicleconfig")
    public List<KnownConfigurationTO> getAllKnownVehicleConfigIds() {
        return dataAccess.getAllKnownVehicleConfigurations()
                         .stream()
                         .map(this::entityToTransport)
                         .collect(Collectors.toList());
    }

    @GET
    @Path("/numberOfConfigs")
    public void triggerGetNumberOfConfigs() {
        vehicleStateClientBean.getService()
                              .getNumberOfConfigs(messageId -> logger.info("Triggered get number of configs with message ID: {}",
                                                                           messageId));
    }

    @GET
    @Path("/getresult")
    public List<GetResultTO> getAllGetResults() {
        return dataAccess.getAllGetResults().stream().map(this::entityToTransport).collect(Collectors.toList());
    }

    @GET
    @Path("/vehicleconfig/{id}")
    public void getById(@Suspended AsyncResponse asyncResponse, @PathParam("id") String id) {
        vehicleStateClientBean.getService().getCurrentConfig(id, messageId -> {
            dataAccess.addGetResult(messageId);
            waitForGetResultBean.waitForGetResult(messageId, vehicleConfiguration -> {
                asyncResponse.resume(vehicleConfiguration);
            }, throwable -> {
                asyncResponse.resume(throwable);
            });
        });
    }

    @GET
    @Path("/callWithException/{message}")
    public void triggerCallWithException(@PathParam("message") String message) {
        vehicleStateClientBean.getService()
                              .callWithExceptionTest(message,
                                                     messageId -> logger.info("Triggered callWithException for {}",
                                                                              messageId));
    }

    @GET
    @Path("/callFireAndForget/{message}")
    public void triggerCallFireAndForget(@PathParam("message") String message) {
        vehicleStateClientBean.getService().callFireAndForgetTest(message);
    }

    private KnownConfigurationTO entityToTransport(KnownVehicleConfiguration entity) {
        return new KnownConfigurationTO(entity.getMessageId(),
                                        entity.getVehicleConfigurationId(),
                                        entity.isSuccessfullyAdded());
    }

    private GetResultTO entityToTransport(GetResult entity) {
        return new GetResultTO(entity.getMessageId(), entity.isFulfilled(), entity.getPayload());
    }
}
