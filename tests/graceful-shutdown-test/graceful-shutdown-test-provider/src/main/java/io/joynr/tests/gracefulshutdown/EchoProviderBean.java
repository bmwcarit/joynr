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
package io.joynr.tests.gracefulshutdown;

import jakarta.ejb.Stateless;
import jakarta.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.exceptions.JoynrIllegalStateException;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.messaging.MessagingQos;
import joynr.tests.graceful.shutdown.EchoSync;

/**
 * Sample implementation of the {@link EchoSync} interface.
 */
@Stateless
@ServiceProvider(serviceInterface = EchoSync.class)
public class EchoProviderBean implements EchoSync {

    private static final Logger logger = LoggerFactory.getLogger(EchoProviderBean.class);

    @Inject
    private SecondLevelServiceClient secondLevelServiceClient;

    @Override
    public String echoString(String data) {
        logger.info("Called with {}", data);
        try {
            MessagingQos messagingQos = new MessagingQos(500L);
            String result = secondLevelServiceClient.get().transform(data, messagingQos);
            logger.info("Returning: {}", result);
            return result;
        } catch (JoynrIllegalStateException e) {
            logger.error("Unable to transform {}. Attempting to report failure.", data, e);
            secondLevelServiceClient.get().logFailure(data);
            String result = "Unable to transform: " + data;
            logger.info("Returning: {}", result);
            return result;
        }
    }
}
