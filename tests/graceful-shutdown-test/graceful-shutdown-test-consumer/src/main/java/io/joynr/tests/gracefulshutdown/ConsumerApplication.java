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

import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.stream.IntStream;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.MessagingQosEffort;
import io.joynr.proxy.Callback;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
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

        io.joynr.proxy.Future<Void> proxyFuture = new io.joynr.proxy.Future<Void>();
        EchoAsync echoService = proxyBuilder.build(new ProxyCreatedCallback<EchoProxy>() {
            @Override
            public void onProxyCreationFinished(EchoProxy result) {
                proxyFuture.resolve();
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                proxyFuture.onFailure(error);
            }
        });
        try {
            proxyFuture.get();
        } catch (Exception e) {
            logger.error("Proxy creation failed", e);
        }
        int numberOfThreads = 3;
        ArrayList<Future<?>> futureList = new ArrayList<Future<?>>();
        ExecutorService executorService = Executors.newFixedThreadPool(numberOfThreads);
        IntStream.range(0, numberOfThreads).forEach(threadCounter -> {
            futureList.add(executorService.submit(() -> {
                while (true) {
                    sendMessage(echoService);
                    sleep(50);
                }
            }));
            sleep(100);
        });
        sleep(86400000);
        // following code is just for sanity, it will not be reached during real test
        // since this program is getting forcibly killed from outside while running
        for (Future<?> future : futureList) {
            future.cancel(false);
        }
        logger.info("ConsumerApplication ended");
    }

    private void sendMessage(EchoAsync echoService) {
        if (!providerPreparingForShutdown.get()) {
            try {
                if (!semaphore.tryAcquire(5, TimeUnit.SECONDS)) {
                    throw new TimeoutException("unable to acquire semaphore in time");
                }
                echoService.echoString(new Callback<String>() {
                    @Override
                    public void onSuccess(String result) {
                        if (result != null) {
                            logger.info("Got echo back: {}", result);
                            if (result.contains("Unable to transform")) {
                                providerPreparingForShutdown.set(true);
                            }
                            semaphore.release();
                        } else {
                            logger.info("Got echo back: null");
                        }
                    }

                    @Override
                    public void onFailure(JoynrRuntimeException runtimeException) {
                        logger.error("Problem calling echo service.", runtimeException);
                        semaphore.release();
                    }
                }, "Test " + System.currentTimeMillis());
            } catch (InterruptedException | TimeoutException e) {
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
