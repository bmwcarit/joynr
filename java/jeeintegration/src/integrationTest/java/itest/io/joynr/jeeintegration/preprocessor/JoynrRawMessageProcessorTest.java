/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2024 BMW Car IT GmbH
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
package itest.io.joynr.jeeintegration.preprocessor;

import static itest.io.joynr.jeeintegration.base.deployment.FileConstants.getBeansXml;
import static itest.io.joynr.jeeintegration.base.deployment.FileConstants.getExtension;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import jakarta.inject.Inject;

import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.arquillian.test.spi.TestResult;
import org.jboss.shrinkwrap.api.Archive;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.JavaArchive;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.Timeout;
import org.junit.runner.RunWith;

import io.joynr.jeeintegration.api.JoynrRawMessagingPreprocessor;
import io.joynr.messaging.RawMessagingPreprocessor;

/**
 * Integration tests for the {@link JoynrRawMessagingPreprocessor}.
 */
@RunWith(Arquillian.class)
public class JoynrRawMessageProcessorTest {
    @Rule
    public Timeout globalTimeout = Timeout.seconds(60);

    @Deployment
    public static Archive<?> getDeployment() {
        return ShrinkWrap.create(JavaArchive.class)
                         .addClasses(JoynrRawMessagingPreprocessor.class,
                                     TestResult.class,
                                     TestRawMessagingProcessorProducer.class,
                                     TestRawMessagingProcessor.class,
                                     RawMessagingPreprocessor.class)
                         .addPackages(true, "org.slf4j")
                         .addAsManifestResource(getBeansXml())
                         .addAsManifestResource(getExtension());
    }

    @Inject
    private @JoynrRawMessagingPreprocessor RawMessagingPreprocessor preprocessor;

    @Test
    public void testRawMessageProcessorRegistered() {
        assertNotNull(preprocessor);
        assertTrue(preprocessor instanceof TestRawMessagingProcessor);
    }
}
