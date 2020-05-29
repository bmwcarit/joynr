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
package itest.io.joynr.jeeintegration;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;

import java.io.File;
import java.net.URL;

import javax.ws.rs.client.ClientBuilder;
import javax.ws.rs.client.WebTarget;
import javax.ws.rs.core.Response;

import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.container.test.api.RunAsClient;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.arquillian.test.api.ArquillianResource;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.JavaArchive;
import org.jboss.shrinkwrap.api.spec.WebArchive;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.Timeout;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.CallbackHandlerDiscovery;
import io.joynr.jeeintegration.DefaultJoynrRuntimeFactory;
import io.joynr.jeeintegration.JeeJoynrServiceLocator;
import io.joynr.jeeintegration.JeeJoynrStatusMetricsAggregator;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.jeeintegration.ServiceProviderDiscovery;
import io.joynr.jeeintegration.messaging.JeeMessagingApplication;
import io.joynr.jeeintegration.messaging.JeeMessagingEndpoint;

/**
 * Integration tests for the {@link JeeMessagingEndpoint}.
 */
@RunWith(Arquillian.class)
@RunAsClient
public class JeeMessagingEndpointTest {
    @Rule
    public Timeout globalTimeout = Timeout.seconds(60);

    private static final Logger logger = LoggerFactory.getLogger(JeeMessagingEndpointTest.class);

    @Deployment
    public static WebArchive createTestArchive() {
        // @formatter:off
        JavaArchive javaArchive = ShrinkWrap.create(JavaArchive.class)
                                            .addClasses(ServiceProviderDiscovery.class,
                                                        CallbackHandlerDiscovery.class,
                                                        DefaultJoynrRuntimeFactory.class,
                                                        JeeIntegrationJoynrTestConfigurationProvider.class,
                                                        JoynrIntegrationBean.class,
                                                        JeeMessagingApplication.class,
                                                        JeeMessagingEndpoint.class,
                                                        JeeJoynrStatusMetricsAggregator.class,
                                                        JeeJoynrServiceLocator.class)
                                            .addAsManifestResource(new File("src/main/resources/META-INF/beans.xml"));
        // @formatter:on
        return ShrinkWrap.create(WebArchive.class).addAsLibraries(javaArchive);
    }

    @ArquillianResource
    private URL baseUrl;

    @Test
    public void testEndpointAvailable() throws Throwable {
        assertNotNull(baseUrl);
        WebTarget webTarget = ClientBuilder.newClient().target(baseUrl.toURI()).path("/messaging/channels");
        logger.info("Trying to access: " + webTarget.getUri().toString());
        Response response = webTarget.request().get();
        assertNotNull(response);
        assertEquals(200, response.getStatus());
    }

}
