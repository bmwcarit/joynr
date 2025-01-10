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
package itest.io.joynr.jeeintegration.multicast;

import static itest.io.joynr.jeeintegration.base.deployment.FileConstants.getBeansXml;
import static itest.io.joynr.jeeintegration.base.deployment.FileConstants.getExtension;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import io.joynr.jeeintegration.api.SubscriptionPublisher;
import jakarta.inject.Inject;

import jakarta.enterprise.inject.spi.Extension;
import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.JavaArchive;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.Timeout;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.jeeintegration.multicast.SubscriptionPublisherCdiExtension;
import io.joynr.jeeintegration.multicast.SubscriptionPublisherInjectionWrapper;
import io.joynr.jeeintegration.multicast.SubscriptionPublisherProducer;
import io.joynr.pubsub.publication.BroadcastFilterImpl;
import joynr.jeeintegration.servicelocator.MyServiceSubscriptionPublisher;
import joynr.jeeintegration.servicelocator.MyServiceSync;

@RunWith(Arquillian.class)
public class SubscriptionPublisherInjectionTest {
    @Rule
    public Timeout globalTimeout = Timeout.seconds(60);

    private static final Logger logger = LoggerFactory.getLogger(SubscriptionPublisherInjectionTest.class);

    @Deployment
    public static JavaArchive getDeployment() {
        return ShrinkWrap.create(JavaArchive.class)
                         .addClasses(SubscriptionPublisherCdiExtension.class,
                                     SubscriptionPublisherInjectionWrapper.class,
                                     SubscriptionPublisherProducer.class,
                                     BeanWithSubscriptionPublisher.class,
                                     SubscriptionPublisher.class,
                                     io.joynr.provider.SubscriptionPublisher.class)
                         .addAsServiceProvider(Extension.class, SubscriptionPublisherCdiExtension.class)
                         .addPackages(true, "joynr.exceptions")
                         .addPackages(true, "joynr.jeeintegration.servicelocator")
                         .addPackages(true, "io.joynr.exceptions")
                         .addPackages(true, "io.joynr.messaging")
                         .addPackages(true, "io.joynr.runtime")
                         .addPackages(true, "io.joynr.subtypes")
                         .addPackages(true, "org.slf4j")
                         .addAsManifestResource(getBeansXml())
                         .addAsManifestResource(getExtension());
    }

    @Inject
    private MyServiceSync beanWithSubscriptionPublisher;

    @Inject
    private SubscriptionPublisherProducer subscriptionPublisherProducer;

    private boolean multicastFired = false;

    private MyServiceSubscriptionPublisher myServiceSubscriptionPublisher = new MyServiceSubscriptionPublisher() {
        @Override
        public void fireMyMulticast(String someValue, String... partitions) {
            logger.info("Multicast fired: {}", someValue);
            multicastFired = true;
        }

        @Override
        public void fireMySelectiveBroadcast(String someValue) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void addBroadcastFilter(BroadcastFilterImpl filter) {
            throw new UnsupportedOperationException();
        }

        @Override
        public void addBroadcastFilter(BroadcastFilterImpl... filters) {
            throw new UnsupportedOperationException();
        }
    };

    @Test
    public void testSubscriptionPublisherSet() {
        assertNotNull(subscriptionPublisherProducer);
        subscriptionPublisherProducer.add(myServiceSubscriptionPublisher, BeanWithSubscriptionPublisher.class);
        assertNotNull(beanWithSubscriptionPublisher);
        assertFalse(multicastFired);
        beanWithSubscriptionPublisher.callMe("Test");
        assertTrue(multicastFired);
    }
}
