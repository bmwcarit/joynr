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
package io.joynr.messaging.bounceproxy.monitoring;

import static org.mockito.Matchers.any;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Properties;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.TimeUnit;

import org.apache.http.HttpException;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClients;
import org.apache.http.localserver.LocalTestServer;
import org.apache.http.protocol.HttpContext;
import org.apache.http.protocol.HttpRequestHandler;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.invocation.InvocationOnMock;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;
import com.google.inject.Injector;

import io.joynr.guice.PropertyLoadingModule;
import io.joynr.messaging.bounceproxy.BounceProxyPropertyKeys;

@RunWith(MockitoJUnitRunner.class)
public class PerformanceReporterTest {

    @Mock
    private ScheduledExecutorService mockExecutorService;

    @Mock
    HttpRequestHandler mockHandler;

    @Mock
    BounceProxyPerformanceMonitor mockPerformanceMonitor;

    @Mock
    BounceProxyLifecycleMonitor mockLifecycleMonitor;

    @Mock
    BounceProxyStartupReporter mockStartupReporter;

    private LocalTestServer server;

    private BounceProxyPerformanceReporter reporter;

    Clock clock;

    @Before
    public void setUp() throws Exception {

        // use local test server to intercept http requests sent by the reporter
        server = new LocalTestServer(null, null);
        server.register("*", mockHandler);
        server.start();

        final String serverUrl = "http://localhost:" + server.getServiceAddress().getPort();

        // set test properties manually for a better overview of
        // what is tested
        Properties properties = new Properties();

        // have to set the BPC base url manually as this is the
        // mock server which gets port settings dynamically
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_CONTROLLER_BASE_URL, serverUrl);
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_ID, "X.Y");
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCEPROXY_URL_FOR_BPC, "http://joyn-bpX.muc/bp");
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCEPROXY_URL_FOR_CC, "http://joyn-bpX.de/bp");
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_MONITORING_FREQUENCY_MS, "100");
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_SEND_LIFECYCLE_REPORT_RETRY_INTERVAL_MS, "500");
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_MAX_SEND_SHUTDOWN_TIME_SECS, "2");

        Injector injector = Guice.createInjector(new PropertyLoadingModule(properties), new AbstractModule() {

            @Override
            protected void configure() {

                bind(ScheduledExecutorService.class).toInstance(mockExecutorService);
                bind(BounceProxyPerformanceMonitor.class).toInstance(mockPerformanceMonitor);
                bind(BounceProxyLifecycleMonitor.class).toInstance(mockLifecycleMonitor);
                bind(CloseableHttpClient.class).toInstance(HttpClients.createDefault());
            }

        });

        reporter = injector.getInstance(BounceProxyPerformanceReporter.class);

        // fake the clock for a more robust testing
        clock = new Clock();

        Answer<Void> mockScheduledExecutor = new Answer<Void>() {

            @Override
            public Void answer(final InvocationOnMock invocation) throws Throwable {

                long frequencyMs = (Long) invocation.getArguments()[2];
                Runnable runnable = (Runnable) invocation.getArguments()[0];

                clock.setFrequencyMs(frequencyMs);
                clock.setRunnable(runnable);

                return null;
            }
        };
        Mockito.doAnswer(mockScheduledExecutor)
               .when(mockExecutorService)
               .scheduleWithFixedDelay(Mockito.any(Runnable.class),
                                       Mockito.anyLong(),
                                       Mockito.anyLong(),
                                       Mockito.any(TimeUnit.class));
    }

    @Test
    public void testStartReporting() throws Exception {

        setMockedHttpResponse();
        Mockito.when(mockLifecycleMonitor.isInitialized()).thenReturn(true);

        reporter.startReporting();

        // monitoring frequency is set to 100ms in the properties above
        Mockito.when(mockPerformanceMonitor.getAsKeyValuePairs()).thenReturn(createPerformanceMap(1, 100));
        clock.tick(100);
        ArgumentCaptor<HttpRequest> argumentFirstCall = ArgumentCaptor.forClass(HttpRequest.class);
        Mockito.verify(mockHandler, Mockito.times(1))
               .handle(argumentFirstCall.capture(), Mockito.any(HttpResponse.class), Mockito.any(HttpContext.class));
        Assert.assertThat(argumentFirstCall.getValue(), MockitoTestUtils.isAnyPerformanceHttpRequest("X.Y", 1, 100));

        Mockito.when(mockPerformanceMonitor.getAsKeyValuePairs()).thenReturn(createPerformanceMap(2, 200));
        clock.tick(200);
        ArgumentCaptor<HttpRequest> argumentSecondCall = ArgumentCaptor.forClass(HttpRequest.class);
        Mockito.verify(mockHandler, Mockito.times(2))
               .handle(argumentSecondCall.capture(), Mockito.any(HttpResponse.class), Mockito.any(HttpContext.class));
        Assert.assertThat(argumentSecondCall.getValue(), MockitoTestUtils.isAnyPerformanceHttpRequest("X.Y", 2, 200));

        Mockito.when(mockPerformanceMonitor.getAsKeyValuePairs()).thenReturn(createPerformanceMap(3, 300));
        clock.tick(300);
        ArgumentCaptor<HttpRequest> argumentThirdCall = ArgumentCaptor.forClass(HttpRequest.class);
        Mockito.verify(mockHandler, Mockito.times(3))
               .handle(argumentThirdCall.capture(), Mockito.any(HttpResponse.class), Mockito.any(HttpContext.class));
        Assert.assertThat(argumentThirdCall.getValue(), MockitoTestUtils.isAnyPerformanceHttpRequest("X.Y", 3, 300));
    }

    @Test
    public void testStartReportingWithOngoingStartupReporting() throws Exception {

        setMockedHttpResponse();
        Mockito.when(mockLifecycleMonitor.isInitialized()).thenReturn(false);

        reporter.startReporting();

        // monitoring frequency is set to 100ms in the properties above
        Mockito.when(mockPerformanceMonitor.getAsKeyValuePairs()).thenReturn(createPerformanceMap(1, 100));
        clock.tick(100);
        Mockito.verifyZeroInteractions(mockHandler);

        Mockito.when(mockPerformanceMonitor.getAsKeyValuePairs()).thenReturn(createPerformanceMap(2, 200));
        clock.tick(200);
        Mockito.verifyZeroInteractions(mockHandler);

        Mockito.when(mockLifecycleMonitor.isInitialized()).thenReturn(true);

        Mockito.when(mockPerformanceMonitor.getAsKeyValuePairs()).thenReturn(createPerformanceMap(3, 300));
        clock.tick(300);
        ArgumentCaptor<HttpRequest> argument = ArgumentCaptor.forClass(HttpRequest.class);
        Mockito.verify(mockHandler, Mockito.times(1))
               .handle(argument.capture(), Mockito.any(HttpResponse.class), Mockito.any(HttpContext.class));
        Assert.assertThat(argument.getValue(), MockitoTestUtils.isAnyPerformanceHttpRequest("X.Y", 3, 300));
    }

    private void setMockedHttpResponse() throws HttpException, IOException {
        // HttpResponse is set as out parameter of the handle method. The way to
        // set out parameters with Mockito is to use doAnswer
        Answer<Void> answerForHttpResponse = MockitoTestUtils.createAnswerForHttpResponse(HttpStatus.SC_NO_CONTENT);
        Mockito.doAnswer(answerForHttpResponse)
               .when(mockHandler)
               .handle(any(HttpRequest.class), any(HttpResponse.class), any(HttpContext.class));
    }

    private Map<String, Integer> createPerformanceMap(int activeLongPolls, int assignedChannels) {
        HashMap<String, Integer> map = new HashMap<String, Integer>();
        map.put("activeLongPolls", activeLongPolls);
        map.put("assignedChannels", assignedChannels);
        return map;
    }

    class Clock {

        private long lastTickMs;

        private long frequencyMs;

        private Runnable runnable;

        public void tick(long tickMs) {
            for (int i = 0; i < (tickMs - lastTickMs) / frequencyMs; i++) {
                runnable.run();
            }
            lastTickMs = tickMs;
        }

        public void setRunnable(Runnable runnable) {
            this.runnable = runnable;
        }

        public void setFrequencyMs(long frequencyMs) {
            this.frequencyMs = frequencyMs;
        }
    }

}
