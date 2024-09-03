/*
 * #%L
 * %%
 * Copyright (C) 2021 - 2024 BMW Car IT GmbH
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
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Semaphore;
import java.util.concurrent.TimeUnit;
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
import io.joynr.exceptions.DiscoveryException;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.performancemeasurement.ApplicationInvocationParameters.TESTCASE;
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
    private static AtomicInteger proxyCreatedCounter = new AtomicInteger(0);

    private static PerformanceTestData performanceTestData;

    private static Thread proxyCreationThread;
    private static ProxyCreationRunnable proxyCreationRunnable;

    private static ApplicationInvocationParameters appInvocationParameters = null;

    public static void main(String[] args) {
        try {
            appInvocationParameters = new ApplicationInvocationParameters(args);
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

        int numOfRequestCalls = appInvocationParameters.getNumberOfRequestCalls();
        int maxRequestInflightCalls = appInvocationParameters.getNumberOfMaxInflightCalls();
        int numOfProxyCreations = appInvocationParameters.getNumberOfProxyCreations();
        String filename = appInvocationParameters.getFileName();
        int numOfIterations = appInvocationParameters.getNumberOfIterations();

        if (appInvocationParameters.getTestCase() == TESTCASE.REQUESTS_ONLY) {
            for (int i = 0; i < numOfIterations; i++) {
                performanceTestData = new PerformanceTestData("Requests only");
                performLookupRequestInLoop(performanceProxy, numOfRequestCalls, maxRequestInflightCalls);
                PerformanceMeasurementStatistics.writeTestDataToCsvFile(performanceTestData, filename);
            }
        } else if (appInvocationParameters.getTestCase() == TESTCASE.REQUESTS_WITH_PROXY) {
            for (int i = 0; i < numOfIterations; i++) {
                performanceTestData = new PerformanceTestData("Requests and proxies");
                performProxiesCreationInLoopInSeparateThread(numOfProxyCreations);
                performLookupRequestInLoop(performanceProxy, numOfRequestCalls, maxRequestInflightCalls);
                shutdownProxyCreationSeparateThread();
                PerformanceMeasurementStatistics.writeTestDataToCsvFile(performanceTestData, filename);
            }
        }
    }

    private static synchronized void performProxiesCreationInLoopInSeparateThread(int numOfCreations) {
        if (proxyCreationRunnable == null) {
            proxyCreationRunnable = new ProxyCreationRunnable();
            proxyCreationRunnable.setNumberOfCalls(numOfCreations);
        }

        proxyCreationThread = new Thread(proxyCreationRunnable);
        proxyCreationThread.start();
    }

    public static synchronized void shutdownProxyCreationSeparateThread() {
        if (proxyCreationRunnable != null) {
            proxyCreationRunnable.stop();
            proxyCreationRunnable = null;
        }
        if (proxyCreationThread != null) {
            try {
                proxyCreationThread.join();
            } catch (InterruptedException e) {
                logger.error("proxyCreationThread.join() interrupted", e);
            }
        }
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
        RequestDurationData durationData = new RequestDurationData(durationQueue, totalDurationMs);
        performanceTestData.setRequestDurationData(durationData);
    }

    private static void shutdown() {
        if (null != provider) {
            try {
                runtime.unregisterProvider(LOCAL_DOMAIN, provider);
            } catch (JoynrRuntimeException exception) {
                logger.error("Failed to unregister provider", exception);
            }
        }

        shutdownProxyCreationSeparateThread();
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

            @Override
            public void onProxyCreationError(DiscoveryException error) {
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

    private static class ProxyCreationRunnable implements Runnable {

        private Logger logger = LoggerFactory.getLogger(ProxyCreationRunnable.class);
        private volatile int numOfProxyCreations = 1;
        private volatile boolean stopped = false;
        private Semaphore proxiesCreationMaxInflightSemaphore = new Semaphore(0);

        public void setNumberOfCalls(int numOfCalls) {
            this.numOfProxyCreations = numOfCalls;
        }

        public void stop() {
            this.stopped = true;
        }

        @Override
        public void run() {
            try {
                proxiesCreationMaxInflightSemaphore = new Semaphore(numOfProxyCreations);

                DiscoveryQos discoveryQos = new DiscoveryQos();
                discoveryQos.setDiscoveryTimeoutMs(10000);
                discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
                discoveryQos.setDiscoveryScope(DiscoveryScope.LOCAL_ONLY);

                while (!stopped) {
                    proxiesCreationMaxInflightSemaphore.acquire();

                    if (stopped) {
                        break;
                    }

                    ProxyBuilder<GlobalCapabilitiesDirectoryProxy> proxyBuilder = runtime.getProxyBuilder(LOCAL_DOMAIN,
                                                                                                          GlobalCapabilitiesDirectoryProxy.class)
                                                                                         .setDiscoveryQos(discoveryQos);

                    proxyBuilder.build(new ProxyCreatedCallback<GlobalCapabilitiesDirectoryProxy>() {

                        @Override
                        public void onProxyCreationFinished(GlobalCapabilitiesDirectoryProxy result) {
                            logger.debug("PerformanceMeasurement: proxy creation in a separate thread succeeded!");
                            proxyCreatedCounter.incrementAndGet();
                            proxiesCreationMaxInflightSemaphore.release();
                        }

                        @Override
                        public void onProxyCreationError(JoynrRuntimeException error) {
                            logger.error("PerformanceMeasurement: proxy creation in a separate thread failed!");
                            proxyCreatedCounter.incrementAndGet();
                            proxiesCreationMaxInflightSemaphore.release();
                        }

                        @Override
                        public void onProxyCreationError(DiscoveryException error) {
                            logger.error("PerformanceMeasurement: proxy creation in a separate thread failed!");
                            proxyCreatedCounter.incrementAndGet();
                            proxiesCreationMaxInflightSemaphore.release();
                        }
                    });
                }
                int numberOfCreatedProxies = proxyCreatedCounter.get();
                performanceTestData.setNumberOfCreatedProxies(numberOfCreatedProxies);
                logger.info("PerformanceMeasurement: wait for proxy creations (created until now {})",
                            numberOfCreatedProxies);
                if (proxiesCreationMaxInflightSemaphore.tryAcquire(5, TimeUnit.SECONDS)) {
                    logger.info("PerformanceMeasurement: proxy creations finished (creations: {})",
                                proxyCreatedCounter.get());
                } else {
                    logger.error("PerformanceMeasurement: proxy creations didn't finish in 5 seconds (created until now {})",
                                 proxyCreatedCounter.get());
                }
            } catch (Exception e) {
                logger.error("PerformanceMeasurement: Unexpected exception in ProxyCreationRunnable: ", e);
            }
        }
    }
}

abstract class AsyncResponseCallback extends Callback<GlobalDiscoveryEntry[]> {
    public abstract void setStartTime(long startTime);
}
