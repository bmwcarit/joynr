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

import com.google.inject.Module;
import com.google.inject.util.Modules;
import io.joynr.accesscontrol.StaticDomainAccessControlProvisioningModule;
import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.messaging.AtmosphereMessagingModule;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.mqtt.paho.client.MqttPahoModule;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.performance.ConsumerInvocationParameters.BackendConfig;
import io.joynr.performance.ConsumerInvocationParameters.COMMUNICATIONMODE;
import io.joynr.performance.ConsumerInvocationParameters.RuntimeConfig;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import jline.internal.Log;
import joynr.tests.performance.EchoProxy;
import joynr.tests.performance.Types.ComplexStruct;

public class ConsumerApplication extends AbstractJoynrApplication {

    private static final String STATIC_PERSISTENCE_FILE = "java-consumer.persistence_file";
    private static final int ASYNCTEST_RESPONSE_SAMPLEINTERVAL_MS = 10; // 10 milliseconds

    private static ConsumerInvocationParameters invocationParameters = null;

    public static void main(String[] args) {

        try {
            invocationParameters = new ConsumerInvocationParameters(args);

            JoynrApplication consumerApp = createJoynrApplication();

            consumerApp.run();
            consumerApp.shutdown();
        } catch (Exception exception) {
            System.err.println(exception.getMessage());
            System.exit(-1);
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
                backendTransportModules = Modules.combine(backendTransportModules, new MqttPahoModule());
            } else {
                // HTTP
                backendTransportModules = Modules.combine(backendTransportModules, new AtmosphereMessagingModule());
            }
        }

        return Modules.override(runtimeModule).with(backendTransportModules);
    }

    @Override
    public void run() {
        EchoProxy echoProxy = createEchoProxy();

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
                Log.error("Unknown test type used");
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
                Log.error("Unknown test type used");
                break;
            }
        } else {
            Log.error("Unknown communication mode used");
        }
    }

    private EchoProxy createEchoProxy() {
        DiscoveryQos discoveryQos = new DiscoveryQos();

        discoveryQos.setDiscoveryTimeoutMs(100000);
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);
        discoveryQos.setDiscoveryScope(invocationParameters.getDiscoveryScope());

        ProxyBuilder<EchoProxy> proxyBuilder = runtime.getProxyBuilder(invocationParameters.getDomainName(),
                                                                       EchoProxy.class);

        return proxyBuilder.setMessagingQos(new MessagingQos(60000)). // 1 minute
                           setDiscoveryQos(discoveryQos).build();
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

    private void performAsyncSendStringTest(EchoProxy proxy) {
        runAsyncSendStringTest(proxy, invocationParameters.getNumberOfWarmupRuns());
        int iterations = invocationParameters.getNumberOfIterations();

        int numFailures = 0;
        long startTime = System.currentTimeMillis();
        for (int i = 0; i < iterations; i++) {
            numFailures += runAsyncSendStringTest(proxy, invocationParameters.getNumberOfRuns());
        }
        long endTime = System.currentTimeMillis();

        printTestResult(endTime, startTime);
        printFailureStatistic(numFailures, invocationParameters.getNumberOfRuns());
    }

    private int runAsyncSendStringTest(EchoProxy proxy, int runs) {
        AsyncResponseCounterCallback<String> responseCallback = new AsyncResponseCounterCallback<String>();
        String inputString = createInputString(invocationParameters.getStringDataLength(), 'x');

        for (int i = 0; i < runs; i++) {
            proxy.echoString(responseCallback, inputString);
        }

        responseCallback.waitForNumberOfResponses(runs, ASYNCTEST_RESPONSE_SAMPLEINTERVAL_MS);

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
        printFailureStatistic(numFailures, invocationParameters.getNumberOfRuns());
    }

    private int runAsyncSendStructTest(EchoProxy proxy, int runs) {
        AsyncResponseCounterCallback<ComplexStruct> responseCallback = new AsyncResponseCounterCallback<ComplexStruct>();
        ComplexStruct inputData = createComplexStructInput();

        for (int i = 0; i < runs; i++) {
            proxy.echoComplexStruct(responseCallback, inputData);
        }

        responseCallback.waitForNumberOfResponses(runs, ASYNCTEST_RESPONSE_SAMPLEINTERVAL_MS);

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
        printFailureStatistic(numFailures, invocationParameters.getNumberOfRuns());
    }

    private int runAsyncSendByteArrayTest(EchoProxy proxy, int runs) {
        AsyncResponseCounterCallback<Byte[]> responseCallback = new AsyncResponseCounterCallback<Byte[]>();
        Byte[] inputData = createInputByteArray(invocationParameters.getByteArraySize(), (byte) 0);

        for (int i = 0; i < runs; i++) {
            proxy.echoByteArray(responseCallback, inputData);
        }

        responseCallback.waitForNumberOfResponses(runs, ASYNCTEST_RESPONSE_SAMPLEINTERVAL_MS);

        return responseCallback.getNumberOfFailures();
    }

    private Byte[] createInputByteArray(int size, byte value) {
        Byte[] result = new Byte[size];

        Arrays.fill(result, new Byte(value));

        return result;
    }

    private String createInputString(int length, char initialChar) {
        char[] data = new char[length];
        Arrays.fill(data, initialChar);

        return new String(data);
    }

    private void printTestResult(long endTime, long startTime) {
        printTestResult(endTime, startTime, 1);
    }

    private void printTestResult(long endTime, long startTime, int iterations) {
        long timeDeltaMilliseconds = endTime - startTime;
        System.err.format("Test case took %d ms. %.2f Msgs/s transmitted\n startTime: %d, endTime: %d, runs: %d, iterations: %d \n",
                          timeDeltaMilliseconds,
                          (double) (invocationParameters.getNumberOfRuns() * iterations)
                                  / ((double) timeDeltaMilliseconds / 1000.0),
                          startTime,
                          endTime,
                          invocationParameters.getNumberOfRuns(),
                          invocationParameters.getNumberOfIterations());
    }

    private <type> void printFailureStatistic(int numFailures, int numRuns) {
        if (numFailures > 0) {
            System.err.format("%d out of %d transmissions failed\n", numFailures, numRuns);
        }
    }

    @Override
    public void shutdown() {
        runtime.shutdown(true);

        // TODO currently there is a bug preventing all threads being stopped: WORKAROUND
        sleep(3000);

        System.exit(0);
    }

    private boolean sleep(long milliseconds) {
        try {
            Thread.sleep(milliseconds);
        } catch (InterruptedException e) {
            return false;
        }

        return true;
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
