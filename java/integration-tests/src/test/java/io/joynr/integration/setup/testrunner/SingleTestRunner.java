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
package io.joynr.integration.setup.testrunner;

import java.lang.reflect.Field;

import org.junit.runner.Description;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;

import io.joynr.integration.setup.BounceProxyServerSetup;

/**
 * Test runner that runs a test class for a single bounce proxy server setup.
 * 
 * @author christina.strobel
 * 
 */
class SingleTestRunner extends BlockJUnit4ClassRunner {

    private BounceProxyServerSetup bounceProxySetup;

    public SingleTestRunner(Class<?> testClass, BounceProxyServerSetup bounceProxySetup) throws InitializationError {
        super(testClass);
        this.bounceProxySetup = bounceProxySetup;
    }

    @Override
    protected String getName() {
        return getTestClass().getJavaClass().getSimpleName() + " [Config: "
                + bounceProxySetup.getClass().getSimpleName() + "]";
    }

    @Override
    protected Description describeChild(FrameworkMethod method) {
        return Description.createTestDescription(getTestClass().getJavaClass().getName(),
                                                 testName(method),
                                                 testName(method) + "-" + bounceProxySetup.getClass().getSimpleName());
    }

    /**
     * Returns the setup for which the test runner runs all tests of the test
     * class.
     * 
     * @return
     */
    public BounceProxyServerSetup getSetup() {
        return bounceProxySetup;
    }

    @Override
    protected Object createTest() throws Exception {
        Object instance = super.createTest();
        injectBounceProxySetup(instance);
        return instance;
    }

    private void injectBounceProxySetup(Object testClassInstance) throws IllegalArgumentException,
                                                                  IllegalAccessException {

        for (Field field : testClassInstance.getClass().getDeclaredFields()) {

            BounceProxyServerContext configuration = field.getAnnotation(BounceProxyServerContext.class);
            if (configuration != null) {
                field.set(testClassInstance, bounceProxySetup);
            }

        }

    }
}
