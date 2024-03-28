/*
 * #%L
 * %%
 * Copyright (C) 2022 BMW Car IT GmbH
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

import java.io.File;

import com.google.inject.Inject;

import org.jboss.arquillian.container.test.api.Deployment;
import org.jboss.arquillian.junit.Arquillian;
import org.jboss.shrinkwrap.api.ShrinkWrap;
import org.jboss.shrinkwrap.api.spec.JavaArchive;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.Timeout;
import org.junit.runner.RunWith;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import static org.mockito.Mockito.verify;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import io.joynr.jeeintegration.CallbackHandlerDiscovery;
import io.joynr.jeeintegration.DefaultJoynrRuntimeFactory;
import io.joynr.jeeintegration.JeeJoynrServiceLocator;
import io.joynr.jeeintegration.JeeJoynrStatusMetricsAggregator;
import io.joynr.jeeintegration.JoynrIntegrationBean;
import io.joynr.jeeintegration.ServiceProviderDiscovery;
import io.joynr.jeeintegration.api.ServiceLocator;
import io.joynr.jeeintegration.multicast.SubscriptionPublisherProducer;
import io.joynr.jeeintegration.api.security.JoynrCallingPrincipal;
import io.joynr.jeeintegration.JoynrJeeMessageMetaInfo;
import io.joynr.proxy.Future;

import joynr.jeeintegration.servicelocator.MyServiceSync;
import joynr.jeeintegration.servicelocator.MyServiceAsync;
import joynr.jeeintegration.servicelocator.MyService.CallMeWithExceptionErrorEnum;

import joynr.test.JoynrTestLoggingRule;
import joynr.exceptions.ApplicationException;
import io.joynr.proxy.CallbackWithModeledError;

/**
 * End2End test for provider proxies
 */
@RunWith(Arquillian.class)
public class ProviderProxyEnd2EndTest {
    private static final Logger logger = LoggerFactory.getLogger(ProviderProxyEnd2EndTest.class);
    @Rule
    public Timeout globalTimeout = Timeout.seconds(60);
    @Rule
    public JoynrTestLoggingRule joynrTestRule = new JoynrTestLoggingRule(logger);

    @Deployment
    public static JavaArchive createTestArchive() {
        // @formatter:off
        return ShrinkWrap.create(JavaArchive.class)
                         .addClasses(ServiceProviderDiscovery.class,
                                     CallbackHandlerDiscovery.class,
                                     SubscriptionPublisherProducer.class,
                                     DefaultJoynrRuntimeFactory.class,
                                     JeeJoynrStatusMetricsAggregator.class,
                                     JeeJoynrServiceLocator.class,
                                     JoynrIntegrationBean.class,
                                     MqttOnlyJoynrConfigurationProvider.class,
                                     TestProvider.class,
                                     MyServiceSync.class,
                                     JoynrCallingPrincipal.class,
                                     JoynrJeeMessageMetaInfo.class)
                         .addAsManifestResource(new File("src/main/resources/META-INF/beans.xml"));
        // @formatter:on
    }

    @Inject
    private ServiceLocator serviceLocator;

    @Mock
    private CallbackWithModeledError<Void, CallMeWithExceptionErrorEnum> callbackWithApplicationException;

    @Before
    public void baseSetup() {
        MockitoAnnotations.initMocks(this);

    }

    @Test
    public void testThrowApplicationExceptionSync() {

        MyServiceSync proxy = serviceLocator.builder(MyServiceSync.class, "io.joynr.jeeintegration").build();

        try {
            proxy.callMeWithException();
            fail("Should throw ApplicationException");
        } catch (ApplicationException e) {
            ApplicationException expected = new ApplicationException(CallMeWithExceptionErrorEnum.MY_ERROR,
                                                                     "ErrorMessage");
            assertEquals(expected, e);
            String expectedMessage = "ErrorMessage, ErrorValue: " + CallMeWithExceptionErrorEnum.MY_ERROR.toString();
            assertEquals(e.getMessage(), expectedMessage);
        } catch (Exception e) {
            fail(e.toString());
        }
    }

    @Test
    public void testThrowApplicationExceptionAsync() {

        MyServiceAsync proxy = serviceLocator.builder(MyServiceAsync.class, "io.joynr.jeeintegration").build();
        Future<Void> future = proxy.callMeWithException(callbackWithApplicationException);
        ApplicationException expected = new ApplicationException(CallMeWithExceptionErrorEnum.MY_ERROR, "ErrorMessage");
        String expectedMessage = "ErrorMessage, ErrorValue: " + CallMeWithExceptionErrorEnum.MY_ERROR.toString();

        try {
            future.get();
            fail("Should throw ApplicationException");
        } catch (ApplicationException e) {
            assertEquals(expected, e);
            assertEquals(e.getMessage(), expectedMessage);
        } catch (Exception e) {
            fail(e.toString());
        }

        verify(callbackWithApplicationException).onFailure((CallMeWithExceptionErrorEnum) (expected.getError()));
    }

}
