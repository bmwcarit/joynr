package io.joynr.messaging.bounceproxy.monitoring;

/*
 * #%L
 * joynr::java::messaging::bounceproxy::controlled-bounceproxy
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

import io.joynr.guice.PropertyLoadingModule;
import io.joynr.messaging.bounceproxy.BounceProxyPropertyKeys;

import java.util.Properties;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;

import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

public class BounceProxyStartupReporterTest {

    private BounceProxyStartupReporter startupReporter;

    private ExecutorService execService;

    @Before
    public void setUp() {

        Properties properties = new Properties();
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_SEND_LIFECYCLE_REPORT_RETRY_INTERVAL_MS, "50");
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_CONTROLLER_BASE_URL, "http://anyurl.com");
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_ID, "X.Y");
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCEPROXY_URL_FOR_BPC, "http://anyurl.com");
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCEPROXY_URL_FOR_CC, "http://anyurl.com");

        Injector injector = Guice.createInjector(new PropertyLoadingModule(properties), new AbstractModule() {

            @Override
            protected void configure() {
                bind(ExecutorService.class).toInstance(Executors.newSingleThreadExecutor());
                bind(CloseableHttpClient.class).toInstance(HttpClients.createMinimal());
            }

        });

        startupReporter = injector.getInstance(BounceProxyStartupReporter.class);
        execService = injector.getInstance(ExecutorService.class);
    }

    @Test
    @Ignore
    public void testCancellationForUnreachableServer() throws Exception {

        // start reporting without server, so that reporting fails
        startupReporter.startReporting();

        Thread.sleep(300);

        Assert.assertFalse(execService.isShutdown());
        Assert.assertFalse(execService.isTerminated());

        startupReporter.cancelReporting();

        Assert.assertFalse(startupReporter.hasBounceProxyBeenRegistered());
        Assert.assertTrue("Tasks completed after shutdown", execService.awaitTermination(15, TimeUnit.SECONDS));
        Assert.assertTrue(execService.isShutdown());
        Assert.assertTrue(execService.isTerminated());
    }

}
