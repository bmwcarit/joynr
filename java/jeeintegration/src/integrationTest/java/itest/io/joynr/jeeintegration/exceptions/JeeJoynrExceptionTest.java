/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
package itest.io.joynr.jeeintegration.exceptions;

import static org.junit.Assert.fail;

import java.io.File;

import javax.inject.Inject;

import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.JavaArchive;
import org.junit.Test;
import org.junit.runner.RunWith;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.jeeintegration.CallbackHandlerDiscovery;
import io.joynr.jeeintegration.DefaultJoynrRuntimeFactory;
import io.joynr.jeeintegration.JeeJoynrServiceLocator;
import io.joynr.jeeintegration.JeeJoynrStatusMetricsAggregator;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.jeeintegration.ServiceProviderDiscovery;
import itest.io.joynr.jeeintegration.JeeIntegrationJoynrTestConfigurationProvider;

@RunWith(Arquillian.class)
public class JeeJoynrExceptionTest {

    @Inject
    private TestBean testBean;

    @Deployment
    public static JavaArchive createTestArchive() {
        return ShrinkWrap.create(JavaArchive.class)
                         .addClasses(ServiceProviderDiscovery.class,
                                     CallbackHandlerDiscovery.class,
                                     DefaultJoynrRuntimeFactory.class,
                                     JeeIntegrationJoynrTestConfigurationProvider.class,
                                     JoynrIntegrationBean.class,
                                     JeeJoynrStatusMetricsAggregator.class,
                                     JeeJoynrServiceLocator.class,
                                     JoynrRuntimeException.class,
                                     TestBean.class)
                         .addAsManifestResource(new File("src/main/resources/META-INF/beans.xml"));
    }

    @Test
    public void testCatchJoynrRuntimeException() {
        try {
            testBean.testMethod();
            fail("Shouldn't get this far.");
        } catch (JoynrRuntimeException e) {
            // Expected
        }
    }
}
