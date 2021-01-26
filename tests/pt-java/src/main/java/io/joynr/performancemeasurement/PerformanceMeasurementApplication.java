/*
 * #%L
 * %%
 * Copyright (C) 2021 BMW Car IT GmbH
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

package io.joynr.performancemeasurement;

import java.util.Properties;
import java.util.Scanner;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Semaphore;
import java.util.concurrent.atomic.AtomicInteger;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Guice;
import com.google.inject.Injector;
import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.capabilities.ParticipantIdKeyUtil;
import io.joynr.common.JoynrPropertiesModule;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrRuntime;
import joynr.exceptions.ApplicationException;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProvider;
import joynr.infrastructure.GlobalCapabilitiesDirectoryProxy;
import joynr.types.GlobalDiscoveryEntry;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class PerformanceMeasurementApplication {
    private static final Logger logger = LoggerFactory.getLogger(PerformanceMeasurementApplication.class);

    private static final String LOCAL_DOMAIN = "persistanceMeasurementEnvironmentDomain";

    private static JoynrRuntime runtime;
    private static PerformanceMeasurementProvider provider = null;

    private static AtomicInteger resultCounterReceived;
    private static ConcurrentLinkedQueue<Long> durationQueue;

    public static void main(String[] args) {
        try {
            runtime = createRuntime();
            run();
            shutdown();
        } catch (Exception exception) {
            System.err.println(exception.getMessage());
            System.exit(1);
        }
    }

    private static JoynrRuntime createRuntime() {
        Properties properties = new Properties();
        properties.put(ParticipantIdKeyUtil.getProviderParticipantIdKey(LOCAL_DOMAIN,
                                                                        GlobalCapabilitiesDirectoryProvider.class),
                       "performanceMeasurementParticipantId");
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule())
                                      .with(new HivemqMqttClientModule(), new JoynrPropertiesModule(properties));
        Injector injector = Guice.createInjector(runtimeModule);
        return injector.getInstance(JoynrRuntime.class);
    }

    private static void run() {
        // Register provider
        provider = new PerformanceMeasurementProvider();
        ProviderQos providerQos = new ProviderQos();
        providerQos.setPriority(System.currentTimeMillis());
        providerQos.setScope(ProviderScope.LOCAL);

        Future<Void> future = runtime.getProviderRegistrar(LOCAL_DOMAIN, provider)
                                     .withProviderQos(providerQos)
                                     .register();

        try {
            future.get(1000);
            logger.info("PerformanceMeasurement: Java provider registration succeeded");
        } catch (JoynrRuntimeException | ApplicationException | InterruptedException e) {
            logger.error("PerformanceMeasurement test failed: Java provider registration failed unexpectedly", e);
            return;
        }

        //Build proxy for registered provider
        GlobalCapabilitiesDirectoryProxy performanceProxy;
        try {
            performanceProxy = createPerformanceMeasurementProxy();
            logger.info("PerformanceMeasurement: proxy creation succeeded");
        } catch (Exception e) {
            logger.error("PerformanceMeasurement failed: proxy creation failed: ", e);
            return;
        }

        int numOfRequestCalls = 10000;
        int maxRequestInflightCalls = 100;

        Scanner scanner = new Scanner(System.in, "UTF-8");
        String key = "";
        while (!key.equals("q")) {
            key = scanner.nextLine();
            switch (key) {
            case "tc1":
                performLookupRequestInLoop(performanceProxy, numOfRequestCalls, maxRequestInflightCalls);
                break;
            default:
                StringBuilder usageStringBuilder = new StringBuilder();
                usageStringBuilder.append("\n\nUSAGE press\n");
                usageStringBuilder.append(" tc1\tperform " + numOfRequestCalls + " lookup requests\n");
                usageStringBuilder.append(" q\tto quit\n");
                logger.info(usageStringBuilder.toString());
                break;
            }
        }
        scanner.close();

    }

    private static void performLookupRequestInLoop(GlobalCapabilitiesDirectoryProxy proxy,
                                                   int numOfCalls,
                                                   int maxInflightCalls) {
        String[] providerDomains = new String[]{ LOCAL_DOMAIN };
        String interfaceName = "infrastructure/GlobalCapabilitiesDirectory";
        Semaphore testPerformedSemaphore = new Semaphore(0);
        Semaphore maxInflightSemaphore = new Semaphore(maxInflightCalls);

        resultCounterReceived = new AtomicInteger(0);
        durationQueue = new ConcurrentLinkedQueue<Long>();

        long startOfTotalDuration = System.currentTimeMillis();
        for (int i = 0; i < numOfCalls; i++) {
            try {
                maxInflightSemaphore.acquire();
            } catch (InterruptedException e) {
                logger.error("PerformanceMeasurement: maxInflightSemaphore.acquire() failed!");
                System.exit(1);
            }
            AsyncResponseCallback lookupCallback = new AsyncResponseCallback() {
                private long mStartTime;

                @Override
                public void setStartTime(long startTime) {
                    this.mStartTime = startTime;
                }

                @Override
                public void onSuccess(GlobalDiscoveryEntry[] result) {
                    logger.debug("PerformanceMeasurement: lookup succeeded");
                    updateRequestDataInCallback(mStartTime);
                    maxInflightSemaphore.release();
                    mStartTime = 0l;
                    if (resultCounterReceived.incrementAndGet() == numOfCalls) {
                        testPerformedSemaphore.release();
                    }
                }

                @Override
                public void onFailure(JoynrRuntimeException runtimeException) {
                    logger.error("PerformanceMeasurement: lookup failed");
                    updateRequestDataInCallback(mStartTime);
                    maxInflightSemaphore.release();
                    mStartTime = 0l;
                    if (resultCounterReceived.incrementAndGet() == numOfCalls) {
                        testPerformedSemaphore.release();
                    }
                }
            };
            lookupCallback.setStartTime(System.currentTimeMillis());
            proxy.lookup(lookupCallback, providerDomains, interfaceName);
        }
        // Use this semaphore to wait until all requests are completed
        try {
            testPerformedSemaphore.acquire();
        } catch (InterruptedException e) {
            logger.error("PerformanceMeasurement: testPerformedSemaphore.acquire() failed!");
            System.exit(1);
        }
        long endOfTotalDuration = System.currentTimeMillis();
        long totalDurationMs = endOfTotalDuration - startOfTotalDuration;
    }

    private static void shutdown() {
        if (null != provider) {
            try {
                runtime.unregisterProvider(LOCAL_DOMAIN, provider);
            } catch (JoynrRuntimeException exception) {
                logger.error("Failed to unregister provider", exception);
            }
        }

        runtime.shutdown(true);
        System.exit(0);
    }

    private static GlobalCapabilitiesDirectoryProxy createPerformanceMeasurementProxy() throws InterruptedException,
                                                                                        ApplicationException {
        DiscoveryQos discoveryQos = new DiscoveryQos();

        discoveryQos.setDiscoveryTimeoutMs(10000);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

        ProxyBuilder<GlobalCapabilitiesDirectoryProxy> proxyBuilder = runtime.getProxyBuilder(LOCAL_DOMAIN,
                                                                                              GlobalCapabilitiesDirectoryProxy.class)
                                                                             .setDiscoveryQos(discoveryQos);

        Future<GlobalCapabilitiesDirectoryProxy> gcdProxyFuture = new Future<>();
        proxyBuilder.build(new ProxyCreatedCallback<GlobalCapabilitiesDirectoryProxy>() {

            @Override
            public void onProxyCreationFinished(GlobalCapabilitiesDirectoryProxy result) {
                gcdProxyFuture.onSuccess(result);
            }

            @Override
            public void onProxyCreationError(JoynrRuntimeException error) {
                gcdProxyFuture.onFailure(error);
            }
        });
        return gcdProxyFuture.get();
    }

    private static void updateRequestDataInCallback(long startTime) {
        long endTime = System.currentTimeMillis();
        long duration = endTime - startTime;
        durationQueue.add(duration);
    }
}

abstract class AsyncResponseCallback extends Callback<GlobalDiscoveryEntry[]> {
    public abstract void setStartTime(long startTime);
}
