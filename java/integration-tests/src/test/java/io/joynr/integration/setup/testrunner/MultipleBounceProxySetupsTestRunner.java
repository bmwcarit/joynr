package io.joynr.integration.setup.testrunner;

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

import io.joynr.integration.setup.BounceProxyServerSetup;

import java.lang.annotation.Annotation;

import org.junit.runner.Description;
import org.junit.runner.Runner;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.model.InitializationError;

/**
 * Special JUnit test runner that allows to run a single test class with
 * multiple bounce proxy server setups. To define the setups, the test class has to
 * be annotated with {@link BounceProxyServerSetups}.
 * <br>
 * For each setup, before running all test methods,
 * {@link BounceProxyServerSetup#startServers()} will be called. After running
 * all test methods, {@link BounceProxyServerSetup#stopServers()} is called.
 * 
 * @author christina.strobel
 * 
 */
public class MultipleBounceProxySetupsTestRunner extends Runner {

    private Class<?> testClass;
    private SingleTestRunner[] testRunners;

    public MultipleBounceProxySetupsTestRunner(Class<?> testClass) throws InitializationError {
        this.testClass = testClass;

        BounceProxyServerSetups annotation = this.testClass.getAnnotation(BounceProxyServerSetups.class);

        Class<? extends BounceProxyServerSetup>[] configurationClasses = annotation.value();
        testRunners = new SingleTestRunner[configurationClasses.length];

        for (int i = 0; i < configurationClasses.length; i++) {

            Class<? extends BounceProxyServerSetup> configClass = configurationClasses[i];

            try {
                BounceProxyServerSetup bpConfiguration = configClass.newInstance();
                testRunners[i] = new SingleTestRunner(testClass, bpConfiguration);

            } catch (InstantiationException e) {
                throw new InitializationError(e);
            } catch (IllegalAccessException e) {
                throw new InitializationError(e);
            }
        }

    }

    private String getName() {
        return testClass.getName() + " [Runner: " + this.getClass().getSimpleName() + "]";
    }

    private Annotation[] getAnnotations() {
        if (testClass == null) {
            return new Annotation[0];
        }
        return testClass.getAnnotations();
    }

    @Override
    public Description getDescription() {

        Description description = Description.createSuiteDescription(getName(), getAnnotations());

        for (SingleTestRunner testRunner : testRunners) {
            Description configDescription = testRunner.getDescription();
            description.addChild(configDescription);
        }

        return description;
    }

    @Override
    public void run(RunNotifier notifier) {

        for (SingleTestRunner testRunner : testRunners) {
            BounceProxyServerSetup bpConfig = testRunner.getSetup();

            try {
                bpConfig.startServers();
                testRunner.run(notifier);
                bpConfig.stopServers();
            } catch (Exception e) {
                notifier.fireTestFailure(new Failure(testRunner.getDescription(), e));
            }
        }
    }
}
