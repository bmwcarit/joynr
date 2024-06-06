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
import java.util.concurrent.ScheduledExecutorService;

import jakarta.annotation.Resource;

import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.arquillian.test.spi.TestResult;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.JavaArchive;
import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.Timeout;
import org.junit.runner.RunWith;

import com.google.inject.Inject;
import com.google.inject.Injector;
import com.google.inject.Key;
import com.google.inject.name.Names;

import io.joynr.capabilities.LocalCapabilitiesDirectory;
import io.joynr.jeeintegration.CallbackHandlerDiscovery;
import io.joynr.jeeintegration.DefaultJoynrRuntimeFactory;
import io.joynr.jeeintegration.JeeJoynrServiceLocator;
import io.joynr.jeeintegration.JeeJoynrStatusMetricsAggregator;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.statusmetrics.JoynrStatusMetrics;
import io.joynr.statusmetrics.JoynrStatusMetricsReceiver;
import io.joynr.jeeintegration.ServiceProviderDiscovery;
import io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys;
import io.joynr.messaging.routing.MessageRouter;
import io.joynr.runtime.JoynrInjectionConstants;

/**
 * Integration tests for the JEE integration bean.
 */
@RunWith(Arquillian.class)
public class JeeIntegrationBeanTest {
    @Rule
    public Timeout globalTimeout = Timeout.seconds(60);

    @Deployment
    public static JavaArchive createTestArchive() {
        // @formatter:off
        return ShrinkWrap.create(JavaArchive.class)
                         .addClasses(ServiceProviderDiscovery.class,
                                     CallbackHandlerDiscovery.class,
                                     DefaultJoynrRuntimeFactory.class,
                                     JeeIntegrationJoynrTestConfigurationProvider.class,
                                     JoynrIntegrationBean.class,
                                     TestResult.class,
                                     JeeJoynrStatusMetricsAggregator.class,
                                     JeeJoynrServiceLocator.class)
                         .addAsManifestResource(new File("src/main/resources/META-INF/beans.xml"));
        // @formatter:on
    }

    @Inject
    private JoynrIntegrationBean joynrIntegrationBean;

    @Inject
    private JoynrStatusMetrics joynrStatusMetrics;

    @Resource(name = JeeIntegrationPropertyKeys.JEE_MESSAGING_SCHEDULED_EXECUTOR_RESOURCE)
    private ScheduledExecutorService scheduledExecutorService;

    @Test
    public void testJoynrRuntimeAvailable() {
        Assert.assertNotNull(joynrIntegrationBean.getRuntime());
    }

    @Test
    public void testManagedScheduledExecutorServiceUsed() {
        assertNotNull(scheduledExecutorService);
        assertNotNull(joynrIntegrationBean);
        Injector joynrInjector = joynrIntegrationBean.getJoynrInjector();
        assertNotNull(joynrInjector);
        assertEquals(scheduledExecutorService,
                     joynrInjector.getInstance(Key.get(ScheduledExecutorService.class,
                                                       Names.named(JoynrInjectionConstants.JOYNR_SCHEDULER_CLEANUP))));
        assertEquals(scheduledExecutorService,
                     joynrInjector.getInstance(Key.get(ScheduledExecutorService.class,
                                                       Names.named(MessageRouter.SCHEDULEDTHREADPOOL))));
        assertEquals(scheduledExecutorService,
                     joynrInjector.getInstance(Key.get(ScheduledExecutorService.class,
                                                       Names.named(LocalCapabilitiesDirectory.JOYNR_SCHEDULER_CAPABILITIES_FRESHNESS))));
    }

    @Test
    public void testJoynrStatusMetricsObjectIsUsedAsJoynrStatusReceiver() {
        Injector joynrInjector = joynrIntegrationBean.getJoynrInjector();

        JoynrStatusMetricsReceiver statusMetricsReceiver = joynrInjector.getInstance(JoynrStatusMetricsReceiver.class);

        assertNotNull(joynrStatusMetrics);

        // We cannot compare these objects directly using the == operator. The reason is that the joynrStatusMetrics object is
        // wrapped by a java proxy. Therefore the objects are different. However, the toString() method is called on the
        // underlying object.
        assertEquals(statusMetricsReceiver.toString(), joynrStatusMetrics.toString());
    }
}
