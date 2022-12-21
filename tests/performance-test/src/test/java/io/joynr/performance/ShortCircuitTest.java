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

import static io.joynr.util.JoynrUtil.createUuidString;
import static org.junit.Assert.assertEquals;

import java.util.Properties;
import java.util.Random;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import com.google.inject.Module;
import com.google.inject.util.Modules;

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.arbitration.DiscoveryScope;
import io.joynr.integration.util.DummyJoynrApplication;
import io.joynr.messaging.routing.TestGlobalAddressModule;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.CCInProcessRuntimeModule;
import io.joynr.runtime.JoynrInjectorFactory;
import io.joynr.runtime.JoynrRuntime;
import joynr.tests.performance.EchoProvider;
import joynr.tests.performance.EchoProxy;
import joynr.tests.performance.Types.ComplexStruct;
import joynr.types.ProviderQos;
import joynr.types.ProviderScope;

public class ShortCircuitTest {

    private static final int THOUSAND = 1000;
    private static final int MILLION = 1000000;
    private static final long CONST_DEFAULT_TEST_TIMEOUT = 60 * THOUSAND;
    private static final String DOMAIN = "performancetests";
    private JoynrRuntime runtime;
    private EchoProxy echoProxy;
    private int times = 1000;

    @Before
    public void setup() throws Exception {
        Module runtimeModule = Modules.override(new CCInProcessRuntimeModule()).with(new TestGlobalAddressModule());
        Properties joynrConfig = new Properties();
        DummyJoynrApplication application = (DummyJoynrApplication) new JoynrInjectorFactory(joynrConfig,
                                                                                             runtimeModule).createApplication(DummyJoynrApplication.class);

        runtime = application.getRuntime();
        DiscoveryQos discoveryQos = new DiscoveryQos(CONST_DEFAULT_TEST_TIMEOUT,
                                                     ArbitrationStrategy.HighestPriority,
                                                     DiscoveryQos.NO_MAX_AGE,
                                                     DiscoveryScope.LOCAL_ONLY);
        ProxyBuilder<EchoProxy> proxyBuilder = runtime.getProxyBuilder(DOMAIN, EchoProxy.class)
                                                      .setDiscoveryQos(discoveryQos);
        echoProxy = proxyBuilder.build();

        EchoProvider echoProvider = new EchoProviderImpl();

        ProviderQos providerQos = new ProviderQos();

        providerQos.setPriority(System.currentTimeMillis());
        providerQos.setScope(ProviderScope.LOCAL);

        runtime.registerProvider(DOMAIN, echoProvider, providerQos);

        // warmup
        for (int i = 0; i < 100; i++) {
            echoProxy.echoString("warmup");
            echoProxy.echoByteArray(new Byte[1]);
        }
    }

    @After
    public void tearDown() throws InterruptedException {
        runtime.shutdown(true);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void roundTripString() throws Exception {
        String sendString = createUuidString();
        long start = System.nanoTime();
        for (int i = 0; i < times; i++) {
            String data = sendString + i;
            String echoString = echoProxy.echoString(data);
            assertEquals(data, echoString);
        }
        long end = System.nanoTime();
        double elapsedTimeMs = (double) (end - start) / MILLION;

        System.err.println("");
        System.err.println("string length:\t" + sendString.length() + "\t# roundtrips: " + times + "\tin time (ms): \t"
                + elapsedTimeMs);
        double averagePerTrip = elapsedTimeMs / times;
        System.err.println("string length:\t" + sendString.length() + "\taverage time (ms) per roundtrip: \t"
                + averagePerTrip);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void roundTripStruct() throws Exception {
        Random random = new Random();
        Long num64 = random.nextLong();
        String str = createUuidString();
        Byte[] byteArray = randomByteArray(10);
        Integer num32 = 0;
        long start = System.nanoTime();
        for (int i = 0; i < times; i++) {
            num32 = random.nextInt();
            ComplexStruct sendStruct = new ComplexStruct(num32, num64, byteArray, str);
            ComplexStruct echoStruct = echoProxy.echoComplexStruct(sendStruct);
            assertEquals(sendStruct, echoStruct);
        }
        long end = System.nanoTime();
        double elapsedTimeMs = (double) (end - start) / MILLION;
        int size = (Integer.SIZE + Long.SIZE) / 8 + str.length() + byteArray.length;

        System.err.println("");
        System.err.println("struct bytes\t" + size + "\t# roundtrips: " + times + "\tin time (ms): \t" + elapsedTimeMs);
        double averagePerTrip = elapsedTimeMs / times;
        System.err.println("struct bytes\t" + size + "\taverage time (ms) per roundtrip: \t" + averagePerTrip);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void roundTrip10KByteArray() throws Exception {
        int byteCount = 10 * THOUSAND;
        double elapsedTimeMs = startSendingBytes(byteCount, times);

        System.err.println("");
        System.err.println("byte[] length:\t" + byteCount + "\t# roundtrips:\t" + times + "\tin time (ms):\t"
                + elapsedTimeMs);
        double averagePerTrip = elapsedTimeMs / times;
        System.err.println("byte[] length:\t" + byteCount + "\taverage time (ms) per roundtrip:\t" + averagePerTrip);
    }

    @Test(timeout = CONST_DEFAULT_TEST_TIMEOUT)
    public void roundTrip100KByteArray() throws Exception {
        int byteCount = 100 * THOUSAND;
        int roundTrips = times / 10;
        double elapsedTimeMs = startSendingBytes(byteCount, roundTrips);

        System.err.println("");
        System.err.println("byte[] length:\t" + byteCount + "\t# roundtrips:\t" + roundTrips + "\tin time (ms):\t"
                + elapsedTimeMs);
        double averagePerTrip = elapsedTimeMs / roundTrips;
        System.err.println("byte[] length:\t" + byteCount + "\taverage time (ms) per roundtrip:\t" + averagePerTrip);
    }

    private double startSendingBytes(int byteCount, int times) {
        Byte[] sendBytes = randomByteArray(byteCount);
        Random random = new Random();
        long start = System.nanoTime();
        for (int i = 0; i < times; i++) {
            byte nextbyte = (byte) random.nextInt();
            sendBytes[0] = nextbyte;
            Byte[] echoBytes = echoProxy.echoByteArray(sendBytes);
            assertEquals(sendBytes[0], echoBytes[0]);
            assertEquals(sendBytes.length, echoBytes.length);
        }
        long end = System.nanoTime();
        double elapsedTimeMs = (double) (end - start) / MILLION;
        return elapsedTimeMs;
    }

    private Byte[] randomByteArray(int byteCount) {
        byte[] bytes = new byte[byteCount];
        new Random().nextBytes(bytes);
        Byte[] sendBytes = new Byte[bytes.length];
        int i = 0;
        for (byte b : bytes) {
            sendBytes[i++] = b;
        }
        return sendBytes;
    }
}
