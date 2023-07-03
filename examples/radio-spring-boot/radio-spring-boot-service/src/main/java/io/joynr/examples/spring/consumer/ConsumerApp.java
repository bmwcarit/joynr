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
package io.joynr.examples.spring.consumer;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrCommunicationException;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.vehicle.RadioProxy;
import joynr.vehicle.RadioStation;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class ConsumerApp extends AbstractJoynrApplication {

    private static final Logger logger = LoggerFactory.getLogger(ConsumerApp.class);

    private RadioProxy radioProxy;

    private boolean working = false;

    @Override
    public void run() {
        final DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(10000);
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_AND_GLOBAL);
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        final ProxyBuilder<RadioProxy> proxyBuilder = runtime.getProxyBuilder(localDomain, RadioProxy.class);

        try {
            radioProxy = proxyBuilder.setMessagingQos(new MessagingQos()).setDiscoveryQos(discoveryQos).build();

            working = true;

            while (working) {
                logger.info("Consumer log entry");
                try {
                    Thread.sleep(1000L);
                } catch (final InterruptedException e) {
                    throw new RuntimeException(e);
                }
            }
        } catch (final DiscoveryException e) {
            logger.error("No provider found", e);
        } catch (final JoynrCommunicationException e) {
            logger.error("The message was not sent: ", e);
        }
    }

    public RadioStation getCurrentRadioStation() {
        if(working) {
            return radioProxy.getCurrentStation();
        } else {
            throw new IllegalStateException("No proxy available");
        }
    }

    public RadioStation shuffleStations() {
        if(working) {
            radioProxy.shuffleStations();
            return radioProxy.getCurrentStation();
        } else {
            throw new IllegalStateException("No proxy available");
        }
    }
}
