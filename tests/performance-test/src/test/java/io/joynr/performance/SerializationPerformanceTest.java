/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
import java.util.Collection;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized.Parameters;
import org.junit.runners.Parameterized;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

import io.joynr.messaging.JsonMessageSerializerModule;
import io.joynr.util.ObjectMapper;
import joynr.Request;

@RunWith(Parameterized.class)
public class SerializationPerformanceTest {

    private ObjectMapper objectMapper;
    private Injector injector;
    private int byteArraySize = 0;
    private String testName = "";
    private byte byteArrayInitValue = 0;
    private int numRuns = 1;

    private final static int NUM_WARMUP_RUNS = 1000;
    private final static int NUM_RUNS = 100;

    public SerializationPerformanceTest(Integer byteArraySize, Integer numRuns, Byte initValue, String testName) {
        this.byteArraySize = byteArraySize.intValue();
        this.numRuns = numRuns;
        this.byteArrayInitValue = initValue.byteValue();
        this.testName = testName;
    }

    @Parameters
    public static Collection<Object[]> data() {
        return Arrays.asList(new Object[][]{
                { Integer.valueOf(1000), Integer.valueOf(NUM_WARMUP_RUNS), Byte.valueOf((byte) 123), "Warmup" },
                { Integer.valueOf(1000), Integer.valueOf(NUM_RUNS), Byte.valueOf((byte) 123), "1k bytes (3 digits)" },
                { Integer.valueOf(10000), Integer.valueOf(NUM_RUNS), Byte.valueOf((byte) 123), "10k bytes (3 digits)" },
                { Integer.valueOf(100000), Integer.valueOf(NUM_RUNS), Byte.valueOf((byte) 123),
                        "100k bytes (3 digits)" },
                { Integer.valueOf(1000), Integer.valueOf(NUM_RUNS), Byte.valueOf((byte) 1), "1k bytes (1 digit)" },
                { Integer.valueOf(10000), Integer.valueOf(NUM_RUNS), Byte.valueOf((byte) 1), "10k bytes (1 digit)" },
                { Integer.valueOf(100000), Integer.valueOf(NUM_RUNS), Byte.valueOf((byte) 1),
                        "100k bytes (1 digit)" } });
    }

    public interface ITestCase {
        void run() throws Exception;
    }

    @Before
    public void setUp() {
        injector = Guice.createInjector(new JsonMessageSerializerModule(), new AbstractModule() {
            @Override
            protected void configure() {
                requestStaticInjection(Request.class);
            }
        });

        objectMapper = injector.getInstance(ObjectMapper.class);
    }

    @Test
    public void serializeByteArray_NonBase64() throws Exception {
        // If the Byte class is used, jackson will serialize the array as a list of comma separated objects.
        // If the native byte type is used, jackson will automatically serialize the array as a base64
        // encoded string.
        final Byte[] content = new Byte[byteArraySize];
        Arrays.fill(content, Byte.valueOf(byteArrayInitValue));

        runSerializationBenchmarkTest(String.format("Non-base64 - encode, %s", testName), new ITestCase() {
            @Override
            public void run() throws Exception {
                objectMapper.writeValueAsString(content);
            }
        });
    }

    @Test
    public void deserializeByteArray_NonBase64() throws Exception {
        // If the Byte class is used, jackson will serialize the array as a list of comma separated objects.
        // If the native byte type is used, jackson will automatically serialize the array as a base64
        // encoded string.
        Byte[] content = new Byte[byteArraySize];
        Arrays.fill(content, Byte.valueOf(byteArrayInitValue));
        final String jsonString = objectMapper.writeValueAsString(content);

        runSerializationBenchmarkTest(String.format("Non-base64 - decode, %s", testName), new ITestCase() {
            @Override
            public void run() throws Exception {
                objectMapper.readValue(jsonString, Byte[].class);
            }
        });
    }

    @Test
    public void serializeByteArray_Base64() throws Exception {
        // If the Byte class is used, jackson will serialize the array as a list of comma separated objects.
        // If the native byte type is used, jackson will automatically serialize the array as a base64
        // encoded string.
        final byte[] content = new byte[byteArraySize];
        Arrays.fill(content, byteArrayInitValue);

        runSerializationBenchmarkTest(String.format("Base64 - encode, %s", testName), new ITestCase() {
            @Override
            public void run() throws Exception {
                objectMapper.writeValueAsString(content);
            }
        });
    }

    @Test
    public void deserializeByteArray_Base64() throws Exception {
        // If the Byte class is used, jackson will serialize the array as a list of comma separated objects.
        // If the native byte type is used, jackson will automatically serialize the array as a base64
        // encoded string.
        byte[] content = new byte[byteArraySize];
        Arrays.fill(content, byteArrayInitValue);
        final String jsonString = objectMapper.writeValueAsString(content);

        runSerializationBenchmarkTest(String.format("Base64 - decode, %s", testName), new ITestCase() {
            @Override
            public void run() throws Exception {
                objectMapper.readValue(jsonString, byte[].class);
            }
        });
    }

    public void runSerializationBenchmarkTest(String name, ITestCase testCase) throws Exception {
        long start = System.nanoTime();
        for (int i = 0; i < numRuns; i++) {
            testCase.run();
        }
        long end = System.nanoTime();

        double deltaMs = (double) (end - start) / 1000000.0;
        double averageRunMs = deltaMs / (double) numRuns;

        System.out.println(String.format("Test case %s: %d runs, average %.3f ms, total %.3f ms",
                                         name,
                                         numRuns,
                                         averageRunMs,
                                         deltaMs));
    }
}
