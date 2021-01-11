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
import static org.mockito.Mockito.verify;

import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.List;
import java.util.Properties;

import org.apache.http.HttpException;
import org.apache.http.HttpRequest;
import org.apache.http.HttpResponse;
import org.apache.http.HttpStatus;
import org.apache.http.NameValuePair;
import org.apache.http.client.utils.URLEncodedUtils;
import org.apache.http.localserver.LocalTestServer;
import org.apache.http.protocol.HttpContext;
import org.apache.http.protocol.HttpRequestHandler;
import org.hamcrest.Description;
import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.ArgumentMatcher;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.runners.MockitoJUnitRunner;
import org.mockito.stubbing.Answer;

import com.google.inject.Guice;
import com.google.inject.Injector;

import io.joynr.guice.PropertyLoadingModule;
import io.joynr.messaging.bounceproxy.BounceProxyPropertyKeys;
import io.joynr.messaging.bounceproxy.ControlledBounceProxyModule;
import io.joynr.messaging.bounceproxy.info.BounceProxyInformationProvider;
import io.joynr.messaging.bounceproxy.service.DefaultBounceProxyChannelServiceImpl;
import io.joynr.messaging.info.BounceProxyInformation;
import io.joynr.messaging.service.ChannelService;
import io.joynr.messaging.system.TimestampProvider;

@RunWith(MockitoJUnitRunner.class)
public class MonitoringServiceClientTest {

    private MonitoringServiceClient reporter;

    @Mock
    HttpRequestHandler handler;

    @Mock
    TimestampProvider mockTimestampProvider;

    private LocalTestServer server;

    @Before
    public void setUp() throws Exception {

        // use local test server to intercept http requests sent by the reporter
        server = new LocalTestServer(null, null);
        server.register("*", handler);
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

        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_SEND_LIFECYCLE_REPORT_RETRY_INTERVAL_MS, "100");
        properties.put(BounceProxyPropertyKeys.PROPERTY_BOUNCE_PROXY_MAX_SEND_SHUTDOWN_TIME_SECS, "1");

        Injector injector = Guice.createInjector(new PropertyLoadingModule(properties),
                                                 new ControlledBounceProxyModule() {
                                                     @Override
                                                     protected void configure() {
                                                         bind(ChannelService.class).to(DefaultBounceProxyChannelServiceImpl.class);
                                                         bind(BounceProxyLifecycleMonitor.class).to(MonitoringServiceClient.class);
                                                         bind(TimestampProvider.class).toInstance(mockTimestampProvider);
                                                         bind(BounceProxyInformation.class).toProvider(BounceProxyInformationProvider.class);
                                                     }
                                                 });

        reporter = injector.getInstance(MonitoringServiceClient.class);
    }

    @After
    public void tearDown() {
        try {
            server.stop();
        } catch (Exception e) {
            // do nothing as we don't want tests to fail only because
            // stopping of the server did not work
        }
    }

    @Test
    public void testNotifyNormalStartupForNewBounceProxy() throws Exception {
        setMockedHttpRequestHandlerResponse(HttpStatus.SC_CREATED);

        reporter.startStartupReporting();

        // startStartupReporting is non-blocking, so wait for a maximum of 5
        // secs to check for results
        int i = 50;
        while (i > 0 && !reporter.hasReportedStartup()) {
            i--;
            Thread.sleep(100);
        }

        verify(handler, Mockito.times(1)).handle(
                                                 Mockito.argThat(new IsAnyStartupHttpRequest("X.Y",
                                                                                             "http://joyn-bpX.de/bp/",
                                                                                             "http://joyn-bpX.muc/bp/")),
                                                 any(HttpResponse.class),
                                                 any(HttpContext.class));
        Assert.assertTrue(reporter.hasReportedStartup());
    }

    @Test
    public void testNotifyNormalStartupForKnownBounceProxy() throws Exception {
        setMockedHttpRequestHandlerResponse(HttpStatus.SC_NO_CONTENT);

        reporter.startStartupReporting();

        // startStartupReporting is non-blocking, so wait for a maximum of 5
        // secs to check for results
        int i = 50;
        while (i > 0 && !reporter.hasReportedStartup()) {
            i--;
            Thread.sleep(100);
        }

        verify(handler, Mockito.times(1)).handle(
                                                 Mockito.argThat(new IsAnyStartupHttpRequest("X.Y",
                                                                                             "http://joyn-bpX.de/bp/",
                                                                                             "http://joyn-bpX.muc/bp/")),
                                                 any(HttpResponse.class),
                                                 any(HttpContext.class));
        Assert.assertTrue(reporter.hasReportedStartup());
    }

    @Test
    public void testNotifyStartupTwice() throws Exception {
        setMockedHttpRequestHandlerResponse(HttpStatus.SC_CREATED);

        reporter.startStartupReporting();
        reporter.startStartupReporting();

        // startStartupReporting is non-blocking, so wait for a maximum of 5
        // secs to check for results
        int i = 50;
        while (i > 0 && !reporter.hasReportedStartup()) {
            i--;
            Thread.sleep(100);
        }

        verify(handler, Mockito.times(1)).handle(
                                                 Mockito.argThat(new IsAnyStartupHttpRequest("X.Y",
                                                                                             "http://joyn-bpX.de/bp/",
                                                                                             "http://joyn-bpX.muc/bp/")),
                                                 any(HttpResponse.class),
                                                 any(HttpContext.class));
        Assert.assertTrue(reporter.hasReportedStartup());
    }

    @Test
    public void testNotifyStartupWhenServerIsUnreachable() throws Exception {

        try {
            server.stop();
        } catch (Exception e) {
            // do nothing as we don't want tests to fail only because
            // stopping of the server did not work
        }
        server.awaitTermination(3000);

        reporter.startStartupReporting();

        Thread.sleep(3000);

        // wait for a certain time, then shut down; this should end startup
        // reporting
        // configuration: maximum time of 1 second, so at tick 1000 it should
        // stop
        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(0l, 0l, 500l, 1000l);
        reporter.reportShutdown();

        Mockito.verifyZeroInteractions(handler);
        Assert.assertFalse(reporter.hasReportedStartup());
    }

    @Test
    public void testNotifyNormalShutdown() throws Exception {
        setMockedHttpRequestHandlerResponse(HttpStatus.SC_NO_CONTENT);

        // configuration: maximum time of 1 second, so at tick 1000 it should
        // stop
        // note: the loop should only be executed once and then succeed, but
        // just in case something goes wrong with the server, we set ticks.
        // Otherwise if server startup fails, the loop would never end as time
        // doesn't progress
        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(0l, 0l, 500l, 1000l);

        reporter.reportShutdown();

        verify(handler, Mockito.times(1)).handle(Mockito.argThat(new IsAnyShutdownHttpRequest("X.Y")),
                                                 any(HttpResponse.class),
                                                 any(HttpContext.class));
    }

    @Test
    public void testNotifyShutdownWithUnexpectedServerResponse() throws Exception {
        setMockedHttpRequestHandlerResponse(HttpStatus.SC_CREATED);
        // configuration: maximum time of 1 second, so at tick 1000 it should
        // stop
        // note: for the first two times we return 0, as it is once called
        // before the while loop
        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(0l, 0l, 500l, 1000l);

        reporter.reportShutdown();

        verify(handler, Mockito.times(2)).handle(Mockito.argThat(new IsAnyShutdownHttpRequest("X.Y")),
                                                 any(HttpResponse.class),
                                                 any(HttpContext.class));
    }

    @Test
    public void testNotifyShutdownWhenServerIsUnreachable() throws Exception {

        try {
            server.stop();
        } catch (Exception e) {
            // do nothing as we don't want tests to fail only because
            // stopping of the server did not work
        }
        server.awaitTermination(3000);

        // configuration: maximum time of 1 second, so at tick 1000 it should
        // stop
        // note: for the first two times we return 0, as it is once called
        // before the while loop
        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(0l, 0l, 500l, 1000l);

        reporter.reportShutdown();

        // after the 4th call and return value 1000l the loop should be ended
        verify(mockTimestampProvider, Mockito.times(4)).getCurrentTime();
    }

    @Test
    public void testReportPerformance() throws Exception {
        setMockedHttpRequestHandlerResponse(HttpStatus.SC_NO_CONTENT);

        reporter.startStartupReporting();

        reporter.startPerformanceReport();

        Thread.sleep(1000);

        // sending report every 100ms in 1 second. For robustness, we test for
        // at least 5 invocations
        ArgumentCaptor<HttpRequest> argument1 = ArgumentCaptor.forClass(HttpRequest.class);
        verify(handler, Mockito.atLeast(5)).handle(argument1.capture(),
                                                   any(HttpResponse.class),
                                                   any(HttpContext.class));
        Assert.assertThat(argument1.getValue(), MockitoTestUtils.isAnyPerformanceHttpRequest("X.Y", 0, 0));

        // Shutdown all threads at the end. If we shutdown before test, the
        // mocked handler is messed up with the shutdown report.
        // configuration: maximum time of 1 second, so at tick 1000 it should
        // stop
        // note: the loop should only be executed once and then succeed, but
        // just in case something goes wrong with the server, we set ticks.
        // Otherwise if server startup fails, the loop would never end as time
        // doesn't progress
        Mockito.when(mockTimestampProvider.getCurrentTime()).thenReturn(0l, 0l, 500l, 1000l);
        reporter.reportShutdown();
    }

    /**
     * Sets the HTTP response returned by the
     * {@link HttpRequestHandler#handle(HttpRequest, HttpResponse, HttpContext)}
     * method.
     *
     * @param httpStatus
     *            the desired HTTP status to be returned as HTTP response
     * @throws HttpException
     * @throws IOException
     */
    private void setMockedHttpRequestHandlerResponse(final int httpStatus) throws HttpException, IOException {

        // HttpResponse is set as out parameter of the handle method. The way to
        // set out parameters with Mockito is to use doAnswer
        Answer<Void> answerForHttpResponse = MockitoTestUtils.createAnswerForHttpResponse(httpStatus);
        Mockito.doAnswer(answerForHttpResponse)
               .when(handler)
               .handle(any(HttpRequest.class), any(HttpResponse.class), any(HttpContext.class));
    }

    /**
     * Argument matcher to allow to only check for certain parts of the HTTP
     * startup request. It will only check if query and header parameters are
     * sent correctly. A full check for the whole header would not be very fault
     * tolerant as it includes timestamps etc.
     *
     * @author christina.strobel
     *
     */
    class IsAnyStartupHttpRequest extends ArgumentMatcher<HttpRequest> {

        private String bpId;
        private String url4cc;
        private Object url4bpc;

        public IsAnyStartupHttpRequest(String bpId, String url4cc, String url4bpc) {
            this.bpId = bpId;
            this.url4cc = url4cc;
            this.url4bpc = url4bpc;
        }

        @Override
        public boolean matches(Object arg) {

            try {
                HttpRequest request = (HttpRequest) arg;

                if (!request.getRequestLine().getMethod().equals("PUT")) {
                    return false;
                }

                // query parameter has to contain right bounce proxy ID
                List<NameValuePair> queryParameters = URLEncodedUtils.parse(new URI(request.getRequestLine().getUri()),
                                                                            "UTF-8");

                for (NameValuePair nameValuePair : queryParameters) {
                    if (nameValuePair.getName().equals("bpid") && !nameValuePair.getValue().equals(this.bpId)) {
                        return false;
                    }

                    if (nameValuePair.getName().equals("url4cc") && !nameValuePair.getValue().equals(this.url4cc)) {
                        return false;
                    }

                    if (nameValuePair.getName().equals("url4bpc") && !nameValuePair.getValue().equals(this.url4bpc)) {
                        return false;
                    }
                }

                return true;

            } catch (URISyntaxException e) {
                return false;
            }
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("HTTP PUT request with ");
            description.appendText("query params: [bpid=" + bpId + "], ");
            description.appendText("header params: [url4cc=" + url4cc + ", url4bpc=" + url4bpc + "]");
        }

    }

    /**
     * Argument matcher to allow to only check for certain parts of the HTTP
     * shutdown request. It will only check if query and header parameters are
     * sent correctly. A full check for the whole header would not be very fault
     * tolerant as it includes timestamps etc.
     *
     * @author christina.strobel
     *
     */
    class IsAnyShutdownHttpRequest extends ArgumentMatcher<HttpRequest> {

        private String bpId;

        public IsAnyShutdownHttpRequest(String bpId) {
            this.bpId = bpId;
        }

        @Override
        public boolean matches(Object arg) {

            try {
                HttpRequest request = (HttpRequest) arg;

                if (!request.getRequestLine().getMethod().equals("PUT")) {
                    return false;
                }

                // path parameter has to contain bpid
                URI uri = URI.create(request.getRequestLine().getUri());
                if (!uri.getPath().endsWith("/" + bpId + "/lifecycle")) {
                    return false;
                }

                // query parameter has to contain status=shutdown
                List<NameValuePair> queryParameters = URLEncodedUtils.parse(new URI(request.getRequestLine().getUri()),
                                                                            "UTF-8");

                for (NameValuePair nameValuePair : queryParameters) {
                    if (nameValuePair.getName().equals("status") && !nameValuePair.getValue().equals("shutdown")) {
                        return false;
                    }
                }

                return true;

            } catch (URISyntaxException e) {
                return false;
            }
        }

        @Override
        public void describeTo(Description description) {
            description.appendText("HTTP request with ");
            description.appendText("path params: [bpid=" + bpId + "], ");
            description.appendText("query params: [status=shutdown]");
        }

    }

}
