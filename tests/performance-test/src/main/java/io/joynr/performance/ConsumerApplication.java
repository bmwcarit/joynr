/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.messaging.MessagingQos;
import io.joynr.messaging.websocket.WebsocketModule;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.AbstractJoynrApplication;
import io.joynr.runtime.JoynrApplication;
import io.joynr.runtime.JoynrApplicationModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.LibjoynrWebSocketRuntimeModule;
import jline.internal.Log;
import io.joynr.performance.ConsumerInvocationParameters.COMMUNICATIONMODE;
import joynr.tests.performance.EchoProxy;
import joynr.tests.performance.Types.ComplexStruct;

public class ConsumerApplication extends AbstractJoynrApplication {

    private static final String STATIC_PERSISTENCE_FILE = "java-consumer.persistence_file";
    private static final int ASYNCTEST_RESPONSE_SAMPLEINTERVAL_MS = 10; // 10 milliseconds

    private static ConsumerInvocationParameters invocationParameters = null;

    public static void main(String[] args) {

        try {
            invocationParameters = new ConsumerInvocationParameters(args);
        } catch (Exception exception) {
            System.err.println(exception.getMessage());
            System.exit(-1);
        }

        JoynrApplication consumerApp = createConsumerApp();

        consumerApp.run();
        consumerApp.shutdown();
    }

    private static JoynrApplication createConsumerApp() {
        Properties appConfig = new Properties();
        Properties joynrConfig = new Properties();
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_HOST, "localhost");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PORT, "4242");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PROTOCOL, "ws");
        joynrConfig.setProperty(WebsocketModule.PROPERTY_WEBSOCKET_MESSAGING_PATH, "");
        joynrConfig.setProperty(MessagingPropertyKeys.PERSISTENCE_FILE, STATIC_PERSISTENCE_FILE);

        Module runtimeModule = new LibjoynrWebSocketRuntimeModule();

        return new JoynrInjectorFactory(joynrConfig, runtimeModule).createApplication(new JoynrApplicationModule(ConsumerApplication.class,
                                                                                                                 appConfig));
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

        return proxyBuilder.setMessagingQos(new MessagingQos(3600000)). // 1 hour
                           setDiscoveryQos(discoveryQos)
                           .build();
    }

    private void performSyncSendStringTest(EchoProxy proxy) {
        runSyncStringTest(proxy, invocationParameters.getNumberOfWarmupRuns());

        long startTime = System.currentTimeMillis();
        runSyncStringTest(proxy, invocationParameters.getNumberOfRuns());
        long endTime = System.currentTimeMillis();

        printTestResult(endTime - startTime);
    }

    private void runSyncStringTest(EchoProxy proxy, int runs) {
        String inputString = createInputString(invocationParameters.getStringDataLength(), 'x');

        for (int i = 0; i < runs; i++) {
            proxy.echoString(inputString);
        }
    }

    private void performAsyncSendStringTest(EchoProxy proxy) {
        runAsyncSendStringTest(proxy, invocationParameters.getNumberOfWarmupRuns());

        long startTime = System.currentTimeMillis();
        int numFailures = runAsyncSendStringTest(proxy, invocationParameters.getNumberOfRuns());
        long endTime = System.currentTimeMillis();

        printTestResult(endTime - startTime);
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

        printTestResult(endTime - startTime);
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

        printTestResult(endTime - startTime);
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

        printTestResult(endTime - startTime);
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

        printTestResult(endTime - startTime);
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

    private void printTestResult(long timeDeltaMilliseconds) {
        System.err.format("Test case took %d ms. %.2f Msgs/s transmitted\n",
                          timeDeltaMilliseconds,
                          (double) invocationParameters.getNumberOfRuns() / ((double) timeDeltaMilliseconds / 1000.0));
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
}
