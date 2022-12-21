/*
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
package io.joynr.test.interlanguage;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.junit.AfterClass;
import org.junit.BeforeClass;
import org.junit.Test;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import joynr.interlanguagetest.TestInterfaceSync.MethodWithMultiplePrimitiveParametersReturned;

public class IltConsumerCompressionTest extends IltConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerCompressionTest.class);

    @BeforeClass
    public static void setUp() throws Exception {
        logger.info("setUp: Entering");
        setupConsumerRuntime(true);
        logger.info("setUp: Leaving");
    }

    @AfterClass
    public static void tearDown() throws InterruptedException {
        logger.info("tearDown: Entering");
        generalTearDown();
        logger.info("tearDown: Leaving");
    }

    /*
     * SYNCHRONOUS METHOD CALLS
     */

    // no check possible other than handling exceptions
    @Test
    public void callMethodWithoutParameters() {
        logger.info(name.getMethodName() + "");
        try {
            testInterfaceProxy.methodWithoutParameters();
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }

    @Test
    public void callMethodWithMultiplePrimitiveParameters() {
        logger.info(name.getMethodName() + "");
        try {
            int intArg = 2147483647;
            float floatArg = 47.11f;
            boolean booleanArg = false;
            MethodWithMultiplePrimitiveParametersReturned result;
            result = testInterfaceProxy.methodWithMultiplePrimitiveParameters(intArg, floatArg, booleanArg);
            assertNotNull(result);
            assertTrue(IltUtil.cmpDouble(result.doubleOut, floatArg));
            assertTrue(result.stringOut.equals(Integer.toString(intArg)));
        } catch (Exception e) {
            fail(name.getMethodName() + " - FAILED - caught unexpected exception: " + e.getMessage());
            return;
        }
        logger.info(name.getMethodName() + " - OK");
    }
}
