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
package io.joynr.test.interlanguage.jee;

import org.junit.After;
import org.junit.AfterClass;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Rule;
import org.junit.rules.TestName;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import joynr.interlanguagetest.TestInterfaceSync;

public abstract class IltConsumerTest {
    private static final Logger logger = LoggerFactory.getLogger(IltConsumerTest.class);

    @Rule
    public TestName name = new TestName();

    private static String providerDomain;

    // no real proxy, but keep the name anyway for now since it is used in test classes
    protected TestInterfaceSync testInterfaceProxy;

    @BeforeClass
    public static void generalSetUp() throws Exception {
        logger.info("generalSetUp: Entering");
        logger.info("generalSetUp: Leaving");
    }

    @Before
    public void setUp() {
        logger.info("setUp: Entering");
        if (testInterfaceProxy == null) {
            logger.info("setUp: testInterfaceProxy == null");
            testInterfaceProxy = IltConsumerHelper.getServiceLocator().get(TestInterfaceSync.class,
                                                                           "joynr-inter-language-test-domain");
        }
        logger.info("setUp: Leaving");
    }

    @AfterClass
    public static void generalTearDown() throws InterruptedException {
        logger.info("generalTearDown: Entering");
        logger.info("generalTearDown: Leaving");
    }

    @After
    public void tearDown() {
    }
}
