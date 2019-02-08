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
package io.joynr.examples.messagepersistence.consumer;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingQosEffort;
import io.joynr.proxy.Callback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import joynr.vehicle.RadioAsync;
import joynr.vehicle.RadioProxy;

public class ConsumerApplication extends AbstractJoynrApplication {
    private static final Logger logger = LoggerFactory.getLogger(ConsumerApplication.class);
    private static final int NUM_REQUEST = 100;
    // 10min in order to have time to stop/restart provider side
    private static final long TTL_MS = 10 * 60000L;

    @Override
    public void run() {
        logger.info("ConsumerApplication running ...");

        ProxyBuilder<RadioProxy> proxyBuilder = runtime.getProxyBuilder("io.joynr.examples.messagepersistence",
                                                                        RadioProxy.class);
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setDiscoveryTimeoutMs(60000L);
        discoveryQos.setRetryIntervalMs(500L);
        proxyBuilder.setDiscoveryQos(discoveryQos);

        MessagingQos messagingQos = new MessagingQos();
        messagingQos.setTtl_ms(TTL_MS);
        messagingQos.setEffort(MessagingQosEffort.NORMAL);

        proxyBuilder.setMessagingQos(messagingQos);
        RadioAsync radioProxy = proxyBuilder.build();

        final CountDownLatch responsesCountDown = new CountDownLatch(NUM_REQUEST);

        for (int i = 0; i < NUM_REQUEST; i++) {
            radioProxy.shuffleStations(new Callback<Void>() {

                @Override
                public synchronized void onFailure(JoynrRuntimeException runtimeException) {
                    responsesCountDown.countDown();
                    logger.error("Unexpected error (" + (NUM_REQUEST - responsesCountDown.getCount()) + "/"
                            + NUM_REQUEST + ")", runtimeException);
                }

                @Override
                public synchronized void onSuccess(Void result) {
                    responsesCountDown.countDown();
                    logger.info("Shuffle stations call was successfully executed ("
                            + (NUM_REQUEST - responsesCountDown.getCount()) + "/" + NUM_REQUEST + ")");
                }
            });
        }

        logger.info("{} requests successfully sent to provider", NUM_REQUEST);

        try {
            if (responsesCountDown.await(TTL_MS, TimeUnit.MILLISECONDS)) {
                logger.info("All {} responses arrived", NUM_REQUEST);
            } else {
                final long remainingResponses = responsesCountDown.getCount();
                logger.error("{} of {} responses arrived ({} did not arrive in time)",
                             NUM_REQUEST - remainingResponses,
                             NUM_REQUEST,
                             remainingResponses);
            }
        } catch (InterruptedException e) {
            logger.info("Consumer application interrupted while waiting for all responses", e);
        }
    }
}
