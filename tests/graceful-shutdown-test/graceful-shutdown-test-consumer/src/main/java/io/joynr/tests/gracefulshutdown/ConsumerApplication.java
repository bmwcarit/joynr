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
package io.joynr.tests.gracefulshutdown;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.stream.IntStream;

import javax.annotation.CheckForNull;

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
import joynr.tests.graceful.shutdown.EchoAsync;
import joynr.tests.graceful.shutdown.EchoProxy;

public class ConsumerApplication extends AbstractJoynrApplication {

    private static final Logger logger = LoggerFactory.getLogger(ConsumerApplication.class);

    private final AtomicBoolean providerPreparingForShutdown = new AtomicBoolean();
    private final Semaphore semaphore = new Semaphore(100);

    @Override
    public void run() {
        logger.info("ConsumerApplication running ...");
        sleep(1000);
        ProxyBuilder<EchoProxy> proxyBuilder = runtime.getProxyBuilder("io.joynr.tests.gracefulshutdown.jee.provider",
                                                                       EchoProxy.class);
        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryScope(DiscoveryScope.GLOBAL_ONLY);
        discoveryQos.setDiscoveryTimeoutMs(3000L);
        discoveryQos.setRetryIntervalMs(500L);
        proxyBuilder.setDiscoveryQos(discoveryQos);

        MessagingQos messagingQos = new MessagingQos();
        messagingQos.setTtl_ms(10000L);
        messagingQos.setEffort(MessagingQosEffort.BEST_EFFORT);
        proxyBuilder.setMessagingQos(messagingQos);

        EchoAsync echoService = proxyBuilder.build();
        int numberOfThreads = 3;
        ExecutorService executorService = Executors.newFixedThreadPool(numberOfThreads);
        IntStream.range(0, numberOfThreads).forEach(threadCounter -> {
            executorService.submit(() -> {
                while (true) {
                    sendMessage(echoService);
                    sleep(50);
                }
            });
            sleep(100);
        });
        while (true) {
            sleep(10);
        }
    }

    private void sendMessage(EchoAsync echoService) {
        if (!providerPreparingForShutdown.get()) {
            try {
                semaphore.tryAcquire(5, TimeUnit.SECONDS);
                echoService.echoString(new Callback<String>() {
                    @Override
                    public void onSuccess(@CheckForNull String result) {
                        logger.info("Got echo back: {}", result);
                        if (result.contains("Unable to transform")) {
                            providerPreparingForShutdown.set(true);
                        }
                        semaphore.release();
                    }

                    @Override
                    public void onFailure(JoynrRuntimeException runtimeException) {
                        logger.error("Problem calling echo service.", runtimeException);
                        semaphore.release();
                    }
                }, "Test " + System.currentTimeMillis());
            } catch (InterruptedException e) {
                logger.error("Unable to acquire semaphore for sending message in time.");
            }
        } else {
            logger.info("Provider is preparing for shutdown. Message not sent.");
        }
    }

    private void sleep(long millis) {
        try {
            Thread.sleep(millis);
        } catch (InterruptedException e) {
            logger.error("Interrupted while sleeping.");
        }

    }

}
