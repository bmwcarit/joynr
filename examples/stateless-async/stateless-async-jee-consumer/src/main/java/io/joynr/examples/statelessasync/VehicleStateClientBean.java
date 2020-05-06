/*-
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

import javax.annotation.PostConstruct;
import javax.ejb.ConcurrencyManagement;
import javax.ejb.ConcurrencyManagementType;
import javax.ejb.Singleton;
import javax.ejb.Startup;
import javax.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.ServiceLocator;
import joynr.examples.statelessasync.VehicleStateStatelessAsync;

@Singleton
@Startup
@ConcurrencyManagement(ConcurrencyManagementType.BEAN)
public class VehicleStateClientBean {

    private static final Logger logger = LoggerFactory.getLogger(VehicleStateClientBean.class);

    @Inject
    private ServiceLocator serviceLocator;

    private VehicleStateStatelessAsync service;

    @PostConstruct
    public void initialise() {
        logger.info("START initialisation of vehicle service proxy.");
        service = serviceLocator.builder(VehicleStateStatelessAsync.class, "io.joynr.examples.statelessasync.carsim")
                                .withUseCase("jee-consumer-test")
                                .build();
        logger.info("FINISHED initialisation vehicle service proxy.");
    }

    public VehicleStateStatelessAsync getService() {
        return service;
    }

}
