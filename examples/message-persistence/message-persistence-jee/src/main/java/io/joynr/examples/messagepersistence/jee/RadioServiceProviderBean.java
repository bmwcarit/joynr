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
package io.joynr.examples.messagepersistence.jee;

import javax.ejb.Stateless;
import javax.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.api.SubscriptionPublisher;
import joynr.exceptions.ApplicationException;
import joynr.vehicle.RadioStation;
import joynr.vehicle.RadioSubscriptionPublisher;
import joynr.vehicle.RadioSync;

@Stateless
@ServiceProvider(serviceInterface = RadioSync.class)
public class RadioServiceProviderBean implements RadioSync {
    private static final Logger logger = LoggerFactory.getLogger(RadioServiceProviderBean.class);

    @Inject
    @SubscriptionPublisher
    private RadioSubscriptionPublisher radioSubscriptionPublisher;

    @Override
    public RadioStation getCurrentStation() {
        logger.error("Not implemented.");
        throw new UnsupportedOperationException();
    }

    @Override
    public void shuffleStations() {
        logger.info("JEE provider - Shuffle stations called.");

        try {
            Thread.sleep(1000); //simulate time-consuming computation
        } catch (InterruptedException e) {
            logger.info("Provider interrupted");
        }
    }

    @Override
    public Boolean addFavoriteStation(RadioStation newFavoriteStation) throws ApplicationException {
        logger.error("Not implemented.");
        throw new UnsupportedOperationException();
    }

    @Override
    public GetLocationOfCurrentStationReturned getLocationOfCurrentStation() {
        logger.error("Not implemented.");
        throw new UnsupportedOperationException();
    }
}
