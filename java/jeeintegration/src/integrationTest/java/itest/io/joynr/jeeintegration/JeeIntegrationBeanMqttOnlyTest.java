/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2025 BMW Car IT GmbH
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

import java.util.concurrent.ScheduledExecutorService;

import io.joynr.jeeintegration.JeeJoynrIntegrationModule;
import io.joynr.jeeintegration.JoynrRuntimeFactory;
import io.joynr.jeeintegration.api.CallbackHandler;
import io.joynr.jeeintegration.api.ProviderRegistrationSettingsFactory;
import io.joynr.jeeintegration.api.ServiceLocator;
import io.joynr.jeeintegration.api.ServiceProvider;
import io.joynr.jeeintegration.messaging.JeeMqttMessageSendingModule;
import io.joynr.jeeintegration.messaging.JeeMqttMessagingSkeletonProvider;
import io.joynr.jeeintegration.messaging.JeeSharedSubscriptionsMqttMessagingSkeleton;
import io.joynr.jeeintegration.messaging.JeeSharedSubscriptionsMqttMessagingSkeletonFactory;
import jakarta.annotation.Resource;
import jakarta.inject.Inject;

import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.arquillian.test.spi.TestResult;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.WebArchive;
import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.Timeout;
import org.junit.runner.RunWith;

import io.joynr.jeeintegration.CallbackHandlerDiscovery;
import io.joynr.jeeintegration.DefaultJoynrRuntimeFactory;
import io.joynr.jeeintegration.JeeJoynrServiceLocator;
import io.joynr.jeeintegration.JeeJoynrStatusMetricsAggregator;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.jeeintegration.ServiceProviderDiscovery;
import io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys;

import static itest.io.joynr.jeeintegration.base.deployment.FileConstants.getBeansXml;
import static itest.io.joynr.jeeintegration.base.deployment.MavenDependencyHelper.getMavenDependencies;

/**
 * Integration tests for the JEE integration bean with the MQTT-only configuration.
 */
@RunWith(Arquillian.class)
public class JeeIntegrationBeanMqttOnlyTest {
    @Rule
    public Timeout globalTimeout = Timeout.seconds(60);

    @Deployment
    public static WebArchive createTestArchive() {
        // @formatter:off
        return ShrinkWrap.create(WebArchive.class, "test.war")
                         .addAsLibraries(getMavenDependencies())
                         .addClasses(ServiceProviderDiscovery.class,
                                     CallbackHandlerDiscovery.class,
                                     DefaultJoynrRuntimeFactory.class,
                                     MqttOnlyJoynrConfigurationProvider.class,
                                     JoynrIntegrationBean.class,
                                     TestResult.class,
                                     JeeJoynrStatusMetricsAggregator.class,
                                     JeeJoynrServiceLocator.class,
                                     JoynrRuntimeFactory.class,
                                     ServiceLocator.class,
                                     ProviderRegistrationSettingsFactory.class,
                                     ServiceProvider.class,
                                     JeeJoynrIntegrationModule.class,
                                     JeeMqttMessageSendingModule.class,
                                     JeeMqttMessagingSkeletonProvider.class,
                                     JeeSharedSubscriptionsMqttMessagingSkeletonFactory.class,
                                     JeeSharedSubscriptionsMqttMessagingSkeleton.class,
                                     CallbackHandler.class)
                         .addPackages(true, "io.joynr.accesscontrol")
                         .addPackages(true, "io.joynr.capabilities")
                         .addPackages(true, "io.joynr.messaging")
                         .addPackages(true, "io.joynr.messaging.mqtt")
                         .addPackages(true, "io.joynr.runtime")
                         .addPackages(true, "com.googlecode.cqengine")
                         .addPackages(true, "com.googlecode.concurrenttrees")
                         .addPackages(true, "com.google.protobuf")
                         .addPackages(true, "org.sqlite")
                         .addPackages(true, "org.antlr")
                         .addPackages(true, "com.esotericsoftware")
                         .addPackages(true, "de.javakaffee.kryoserializers")
                         .addPackages(true, "com.github.andrewoma.dexx")
                         .addPackages(true, "org.joda.time")
                         .addPackages(true, "org.apache.wicket.util")
                         .addPackages(true, "net.sf.cglib.proxy")
                         .addAsWebInfResource(getBeansXml());
        // @formatter:on
    }

    @Inject
    private JoynrIntegrationBean joynrIntegrationBean;

    @Resource(name = JeeIntegrationPropertyKeys.JEE_MESSAGING_SCHEDULED_EXECUTOR_RESOURCE)
    private ScheduledExecutorService scheduledExecutorService;

    @Test
    public void testJoynrRuntimeAvailable() {
        Assert.assertNotNull(joynrIntegrationBean.getRuntime());
    }

}
