/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
package io.joynr.performance;

import java.util.Arrays;
import java.util.Properties;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.accesscontrol.StaticDomainAccessControlProvisioningModule;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.hivemq.client.HivemqMqttClientModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.performance.ConsumerInvocationParameters.BackendConfig;
import io.joynr.performance.ConsumerInvocationParameters.COMMUNICATIONMODE;
import io.joynr.performance.ConsumerInvocationParameters.RuntimeConfig;
import io.joynr.proxy.Future;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.proxy.ProxyBuilder.ProxyCreatedCallback;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import joynr.exceptions.ApplicationException;
import joynr.tests.performance.EchoProxy;
import joynr.tests.performance.Types.ComplexStruct;

public class ConsumerApplication extends AbstractJoynrApplication {
    private static final Logger logger = LoggerFactory.getLogger(ConsumerApplication.class);
    private static final String STATIC_PERSISTENCE_FILE = "java-consumer.persistence_file";
    private static final int ASYNCTEST_RESPONSE_SAMPLEINTERVAL_MS = 10; // 10 milliseconds

    private static ConsumerInvocationParameters invocationParameters = null;

    private int exitCode = 0;

    public static void main(String[] args) {

        try {
            invocationParameters = new ConsumerInvocationParameters(args);

            JoynrApplication consumerApp = createJoynrApplication();

            consumerApp.run();
            consumerApp.shutdown();
        } catch (Exception exception) {
            logger.error("Unexpected exception: ", exception);
            System.exit(1);
        }
    }

    private static Module getRuntimeModule(Properties joynrConfig) {

        Module runtimeModule;
        Module backendTransportModules = Modules.EMPTY_MODULE;

        if (invocationParameters.getRuntimeMode() == RuntimeConfig.WEBSOCKET) {
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST,
                                    invocationParameters.getCcHost());
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT,
                                    invocationParameters.getCcPort());
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
            joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
            joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);

            runtimeModule = new LibjoynrWebSocketRuntimeModule();
        } else {
            runtimeModule = new CCInProcessRuntimeModule();
            if (invocationParameters.getBackendTransportMode() == BackendConfig.MQTT) {
                joynrConfig.put("joynr.messaging.mqtt.brokerUri", invocationParameters.getMqttBrokerUri());
                joynrConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
                backendTransportModules = Modules.combine(backendTransportModules, new HivemqMqttClientModule());
            } else {
                // HTTP
                backendTransportModules = Modules.combine(backendTransportModules, new AtmosphereMessagingModule());
            }
        }

        return Modules.override(runtimeModule).with(backendTransportModules);
    }

    @Override
    public void run() {
        EchoProxy echoProxy;
        try {
            echoProxy = createEchoProxy();
        } catch (Exception e) {
            logger.error("Proxy creation failed: ", e);
            exitCode = 1;
            return;
        }

        if (invocationParameters.getCommunicationMode() == COMMUNICATIONMODE.SYNC) {
            switch (invocationParameters.getTestCase()) {
            case SEND_STRING:
                performSyncSendStringTest(echoProxy);
                break;
            case SEND_STRUCT:
                performSyncSendStructTest(echoProxy);
                break;
            case SEND_BYTEARRAY:
                performSyncSendByteArrayTest(echoProxy);
                break;
            default:
                logger.error("Unknown test type used");
                exitCode = 1;
                break;
            }
        } else if (invocationParameters.getCommunicationMode() == COMMUNICATIONMODE.ASYNC) {
            switch (invocationParameters.getTestCase()) {
            case SEND_STRING:
                performAsyncSendStringTest(echoProxy);
                break;
            case SEND_STRUCT:
                performAsyncSendStructTest(echoProxy);
                break;
            case SEND_BYTEARRAY:
                performAsyncSendByteArrayTest(echoProxy);
                break;
            default:
                logger.error("Unknown test type used");
                exitCode = 1;
                break;
            }
        } else {
            logger.error("Unknown communication mode used");
            exitCode = 1;
        }
    }

    private EchoProxy createEchoProxy() throws InterruptedException, ApplicationException {
        DiscoveryQos discoveryQos = new DiscoveryQos();

        discoveryQos.setDiscoveryTimeoutMs(100000);
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setDiscoveryScope(invocationParameters.getDiscoveryScope());

        ProxyBuilder<EchoProxy> proxyBuilder = runtime.getProxyBuilder(invocationParameters.getDomainName(),
                                                                       EchoProxy.class);

        Future<EchoProxy> echoProxyFuture = new Future<>();
        proxyBuilder.setMessagingQos(new MessagingQos(60000, invocationParameters.getEffort())). // 1 minute
                    setDiscoveryQos(discoveryQos).build(new ProxyCreatedCallback<EchoProxy>() {

                        @Override
                        public void onProxyCreationFinished(EchoProxy result) {
                            echoProxyFuture.onSuccess(result);
                        }

                        @Override
                        public void onProxyCreationError(JoynrRuntimeException error) {
                            echoProxyFuture.onFailure(error);
                        }
                    });
        return echoProxyFuture.get();
    }

    private void performSyncSendStringTest(EchoProxy proxy) {
        runSyncStringTest(proxy, invocationParameters.getNumberOfWarmupRuns());

        long startTime = System.currentTimeMillis();
        runSyncStringTest(proxy, invocationParameters.getNumberOfRuns());
        long endTime = System.currentTimeMillis();

        printTestResult(endTime, startTime);
    }

    private void runSyncStringTest(EchoProxy proxy, int runs) {
        String inputString = createInputString(invocationParameters.getStringDataLength(), 'x');

        for (int i = 0; i < runs; i++) {
            proxy.echoString(inputString);
        }
    }

    private static class EchoRunnable implements Runnable {
        private EchoProxy proxy;
        private AtomicLong sentRequests;
        private final int runs;
        private final String inputString;
        private AsyncResponseCounterCallback<String> responseCallback;

        EchoRunnable(EchoProxy proxy,
                     AtomicLong sentRequests,
                     final int runs,
                     final String inputString,
                     AsyncResponseCounterCallback<String> responseCallback) {
            this.proxy = proxy;
            this.sentRequests = sentRequests;
            this.runs = runs;
            this.inputString = inputString;
            this.responseCallback = responseCallback;
        }

        @Override
        public void run() {
            try {
                while (sentRequests.get() < runs) {
                    responseCallback.acquire();
                    if (sentRequests.incrementAndGet() > runs) {
                        sentRequests.decrementAndGet();
                        break;
                    }
                    proxy.echoString(responseCallback, inputString);
                }
            } catch (Exception e) {
                System.err.println("ERROR in Runnable: " + e);
            }
        }

    }

    private void performAsyncSendStringTest(EchoProxy proxy) {
        runAsyncSendStringTest(proxy, invocationParameters.getNumberOfWarmupRuns());
        int iterations = invocationParameters.getNumberOfIterations();
        int runs = invocationParameters.getNumberOfRuns();

        int numFailures = 0;
        long startTime;
        if (invocationParameters.constantNumberOfPendingRequests()) {
            int pendingRequests = invocationParameters.getNumberOfPendingRequests();
            int numThreads = invocationParameters.getNumberOfThreads();
            System.err.format("CNR runs: %d, pending: %d, threads: %d%n", runs, pendingRequests, numThreads);

            startTime = System.currentTimeMillis();
            for (int i = 0; i < iterations; i++) {
                numFailures += runAsyncSendStringTestWithConstantNumberOfPendingRequests(proxy,
                                                                                         runs,
                                                                                         pendingRequests,
                                                                                         numThreads);
            }
        } else {
            startTime = System.currentTimeMillis();
            for (int i = 0; i < iterations; i++) {
                numFailures += runAsyncSendStringTest(proxy, runs);
            }
        }
        long endTime = System.currentTimeMillis();

        printTestResult(endTime, startTime);
        printFailureStatistic(numFailures,
                              invocationParameters.getNumberOfRuns(),
                              invocationParameters.getNumberOfIterations());
    }

    private int runAsyncSendStringTestWithConstantNumberOfPendingRequests(EchoProxy proxy,
                                                                          int runs,
                                                                          int pendingRequests,
                                                                          int numThreads) {
        AsyncResponseCounterCallback<String> responseCallback = new AsyncResponseCounterCallback<String>();
        String inputString = createInputString(invocationParameters.getStringDataLength(), 'x');

        AtomicLong sentRequests = new AtomicLong(pendingRequests);

        ExecutorService executorService = Executors.newFixedThreadPool(numThreads);
        for (int t = 0; t < numThreads; t++) {
            executorService.execute(new EchoRunnable(proxy, sentRequests, runs, inputString, responseCallback));
        }

        for (int j = 0; j < pendingRequests; j++) {
            proxy.echoString(responseCallback, inputString);
        }

        responseCallback.waitForNumberOfResponses(runs, ASYNCTEST_RESPONSE_SAMPLEINTERVAL_MS);

        responseCallback.release(numThreads);
        executorService.shutdown();
        try {
            if (!executorService.awaitTermination(1000, TimeUnit.MILLISECONDS)) {
                System.err.println("ERROR: ExecutorService did not shutdown in time.");
                executorService.shutdownNow();
            }
        } catch (InterruptedException e) {
            System.err.println("ERROR: ExecutorService shutdown interrupted: " + e);
        }
        return responseCallback.getNumberOfFailures();
    }

    private int runAsyncSendStringTest(EchoProxy proxy, int runs) {
        AsyncResponseCounterCallback<String> responseCallback = new AsyncResponseCounterCallback<String>();
        String inputString = createInputString(invocationParameters.getStringDataLength(), 'x');

        for (int i = 0; i < runs; i++) {
            proxy.echoString(responseCallback, inputString);
        }

        responseCallback.waitForNumberOfResponses(runs);

        return responseCallback.getNumberOfFailures();
    }

    private void performSyncSendStructTest(EchoProxy proxy) {
        runSyncSendStructTest(proxy, invocationParameters.getNumberOfWarmupRuns());

        long startTime = System.currentTimeMillis();
        runSyncSendStructTest(proxy, invocationParameters.getNumberOfRuns());
        long endTime = System.currentTimeMillis();

        printTestResult(endTime, startTime);
    }

    private void runSyncSendStructTest(EchoProxy proxy, int runs) {
        ComplexStruct inputData = createComplexStructInput();

        for (int i = 0; i < runs; i++) {
            proxy.echoComplexStruct(inputData);
        }
    }

    private void performAsyncSendStructTest(EchoProxy proxy) {
        runAsyncSendStructTest(proxy, invocationParameters.getNumberOfWarmupRuns());

        long startTime = System.currentTimeMillis();
        int numFailures = runAsyncSendStructTest(proxy, invocationParameters.getNumberOfRuns());
        long endTime = System.currentTimeMillis();

        printTestResult(endTime, startTime);
        printFailureStatistic(numFailures, invocationParameters.getNumberOfRuns(), 1);
    }

    private int runAsyncSendStructTest(EchoProxy proxy, int runs) {
        AsyncResponseCounterCallback<ComplexStruct> responseCallback = new AsyncResponseCounterCallback<ComplexStruct>();
        ComplexStruct inputData = createComplexStructInput();

        for (int i = 0; i < runs; i++) {
            proxy.echoComplexStruct(responseCallback, inputData);
        }

        responseCallback.waitForNumberOfResponses(runs);

        return responseCallback.getNumberOfFailures();
    }

    private ComplexStruct createComplexStructInput() {
        ComplexStruct result = new ComplexStruct();

        result.setData(createInputByteArray(invocationParameters.getByteArraySize(), (byte) 0));
        result.setNum32(1234);
        result.setNum64(42L);
        result.setStr(createInputString(invocationParameters.getStringDataLength(), 'x'));

        return result;
    }

    private void performSyncSendByteArrayTest(EchoProxy proxy) {
        runSyncSendByteArrayTest(proxy, invocationParameters.getNumberOfWarmupRuns());

        long startTime = System.currentTimeMillis();
        runSyncSendByteArrayTest(proxy, invocationParameters.getNumberOfRuns());
        long endTime = System.currentTimeMillis();

        printTestResult(endTime, startTime);
    }

    private void runSyncSendByteArrayTest(EchoProxy proxy, int runs) {
        Byte[] inputData = createInputByteArray(invocationParameters.getByteArraySize(), (byte) 0);

        for (int i = 0; i < runs; i++) {
            proxy.echoByteArray(inputData);
        }
    }

    private void performAsyncSendByteArrayTest(EchoProxy proxy) {
        runAsyncSendByteArrayTest(proxy, invocationParameters.getNumberOfWarmupRuns());

        long startTime = System.currentTimeMillis();
        int numFailures = runAsyncSendByteArrayTest(proxy, invocationParameters.getNumberOfRuns());
        long endTime = System.currentTimeMillis();

        printTestResult(endTime, startTime);
        printFailureStatistic(numFailures, invocationParameters.getNumberOfRuns(), 1);
    }

    private int runAsyncSendByteArrayTest(EchoProxy proxy, int runs) {
        AsyncResponseCounterCallback<Byte[]> responseCallback = new AsyncResponseCounterCallback<Byte[]>();
        Byte[] inputData = createInputByteArray(invocationParameters.getByteArraySize(), (byte) 0);

        for (int i = 0; i < runs; i++) {
            proxy.echoByteArray(responseCallback, inputData);
        }

        responseCallback.waitForNumberOfResponses(runs);

        return responseCallback.getNumberOfFailures();
    }

    private Byte[] createInputByteArray(int size, byte value) {
        Byte[] result = new Byte[size];

        Arrays.fill(result, Byte.valueOf(value));

        return result;
    }

    private String createInputString(int length, char initialChar) {
        char[] data = new char[length];
        Arrays.fill(data, initialChar);

        return new String(data);
    }

    private void printTestResult(long endTime, long startTime) {
        long timeDeltaMilliseconds = endTime - startTime;
        System.err.format("Test case took %d ms. %.2f Msgs/s transmitted%n startTime: %d, endTime: %d, runs: %d, iterations: %d%n",
                          timeDeltaMilliseconds,
                          (invocationParameters.getNumberOfRuns() * invocationParameters.getNumberOfIterations())
                                  / ((timeDeltaMilliseconds) / 1000.0d),
                          startTime,
                          endTime,
                          invocationParameters.getNumberOfRuns(),
                          invocationParameters.getNumberOfIterations());
    }

    private <type> void printFailureStatistic(int numFailures, int numRuns, int iterations) {
        if (numFailures > 0) {
            exitCode = 1;
            System.err.format("%d out of %d transmissions failed%n", numFailures, numRuns * iterations);
        }
    }

    @Override
    public void shutdown() {
        runtime.shutdown(true);

        System.exit(exitCode);
    }

    private static JoynrApplication createJoynrApplication() throws Exception {
        Properties joynrConfig = createJoynrConfig();
        Module runtimeModule = getRuntimeModule(joynrConfig);

        Properties appConfig = new Properties();

        JoynrInjectorFactory injectorFactory = new JoynrInjectorFactory(joynrConfig,
                                                                        runtimeModule,
                                                                        new StaticDomainAccessControlProvisioningModule());

        JoynrApplication joynrApplication = injectorFactory.createApplication(new JoynrApplicationModule(ConsumerApplication.class,
                                                                                                         appConfig));

        return joynrApplication;
    }

    private static Properties createJoynrConfig() throws Exception {
        Properties joynrConfig = new Properties();

        if (invocationParameters.getBackendTransportMode() == BackendConfig.MQTT) {
            joynrConfig.put("joynr.messaging.mqtt.brokerUri", invocationParameters.getMqttBrokerUri());
            joynrConfig.put(MessagingPropertyKeys.PROPERTY_MESSAGING_PRIMARYGLOBALTRANSPORT, "mqtt");
        }

        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);
        joynrConfig.setProperty(PROPERTY_JOYNR_DOMAIN_LOCAL, invocationParameters.getDomainName());

        return joynrConfig;
    }
}
