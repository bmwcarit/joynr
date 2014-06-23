package io.joynr.logging;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import static org.hamcrest.core.StringContains.containsString;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThat;
import static org.mockito.Matchers.eq;
import static org.mockito.Matchers.isA;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.JoynrRuntime;

import java.io.Serializable;
import java.util.List;
import java.util.Map;

import joynr.system.JoynrLogEvent;
import joynr.system.LoggingProxy;

import org.apache.logging.log4j.Level;
import org.apache.logging.log4j.LogManager;
import org.apache.logging.log4j.core.Appender;
import org.apache.logging.log4j.core.Filter;
import org.apache.logging.log4j.core.Layout;
import org.apache.logging.log4j.core.Logger;
import org.apache.logging.log4j.core.LoggerContext;
import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestName;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.runners.MockitoJUnitRunner;

import com.google.inject.AbstractModule;
import com.google.inject.Guice;

@RunWith(MockitoJUnitRunner.class)
public class TestDistributedLoggingAppender {
    @Rule
    public TestName name = new TestName();
    private LoggingProxy loggingProxy;

    LoggerContext context = (LoggerContext) LogManager.getContext();
    Logger logger;

    @Before
    public void setUp() throws Exception {
        final JoynrRuntime runtime = mock(JoynrRuntime.class);
        @SuppressWarnings("unchecked")
        ProxyBuilder<LoggingProxy> proxyBuilder = (ProxyBuilder<LoggingProxy>) mock(ProxyBuilder.class);
        loggingProxy = mock(LoggingProxy.class);
        when(runtime.getProxyBuilder(isA(String.class), eq(LoggingProxy.class))).thenReturn(proxyBuilder);
        when(proxyBuilder.setMessagingQos(isA(MessagingQos.class))).thenReturn(proxyBuilder);
        when(proxyBuilder.setDiscoveryQos(isA(DiscoveryQos.class))).thenReturn(proxyBuilder);
        when(proxyBuilder.build()).thenReturn(loggingProxy);

        // NOTE: Each test has a corresponding log4j .json properties file.
        Guice.createInjector(new AbstractModule() {

            @Override
            protected void configure() {
                bind(JoynrRuntime.class).toInstance(runtime);
                requestStaticInjection(JoynrAppenderManagerFactory.class);
            }
        });

        logger = context.getLogger(name.getMethodName());

    }

    @After
    public void tearDown() throws InterruptedException {
        Map<String, Appender> appenders = logger.getAppenders();
        for (final Map.Entry<String, Appender> entry : appenders.entrySet()) {
            final Appender appender = entry.getValue();
            logger.removeAppender(appender);
            appender.stop();
        }
    }

    @Test
    public void testAppenderFlushesImmediately() throws InterruptedException {
        Layout<? extends Serializable> layout = null;
        String domain = "domain";
        String flushPeriodSecondsAttribute = "0";
        String messageTtlMsAttribute = "1000";
        String discoveryTtlMsAttribute = "1000";
        Filter filter = null;
        String ignoreExceptionsAttribute = "true";
        DistributedLoggingAppender appender = DistributedLoggingAppender.createAppender(name.getMethodName(),
                                                                                        layout,
                                                                                        domain,
                                                                                        flushPeriodSecondsAttribute,
                                                                                        messageTtlMsAttribute,
                                                                                        discoveryTtlMsAttribute,
                                                                                        filter,
                                                                                        ignoreExceptionsAttribute);
        appender.start();
        logger.addAppender(appender);
        logger.setAdditive(false);
        logger.setLevel(Level.DEBUG);

        logger.debug("test1");
        logger.debug("test2");
        logger.debug("test3");

        Thread.sleep(200);

        @SuppressWarnings("unchecked")
        ArgumentCaptor<List<JoynrLogEvent>> argument = ArgumentCaptor.forClass((Class) List.class);

        verify(loggingProxy, times(3)).log(argument.capture());
        List<List<JoynrLogEvent>> capturedLoggingEvents = argument.getAllValues();
        assertThat(capturedLoggingEvents.get(0).get(0).getMessage(), containsString("test1"));
        assertThat(capturedLoggingEvents.get(1).get(0).getMessage(), containsString("test2"));
        assertThat(capturedLoggingEvents.get(2).get(0).getMessage(), containsString("test3"));
    }

    @Test
    public void testAppenderWritesAllEventsToProxy() throws InterruptedException {
        Layout<? extends Serializable> layout = null;
        String domain = "domain";
        String flushPeriodSecondsAttribute = "1";
        String messageTtlMsAttribute = "1000";
        String discoveryTtlMsAttribute = "1000";
        Filter filter = null;
        String ignoreExceptionsAttribute = "true";
        DistributedLoggingAppender appender = DistributedLoggingAppender.createAppender(name.getMethodName(),
                                                                                        layout,
                                                                                        domain,
                                                                                        flushPeriodSecondsAttribute,
                                                                                        messageTtlMsAttribute,
                                                                                        discoveryTtlMsAttribute,
                                                                                        filter,
                                                                                        ignoreExceptionsAttribute);

        appender.start();
        logger.addAppender(appender);
        logger.setAdditive(false);
        logger.setLevel(Level.DEBUG);

        logger.debug("test1");
        logger.debug("test2");
        logger.debug("test3");

        Thread.sleep(1010);

        @SuppressWarnings("unchecked")
        ArgumentCaptor<List<JoynrLogEvent>> argument = ArgumentCaptor.forClass((Class) List.class);
        verify(loggingProxy, times(1)).log(argument.capture());
        assertThat(argument.getValue().get(0).getMessage(), containsString("test1"));
        assertThat(argument.getValue().get(1).getMessage(), containsString("test2"));
        assertThat(argument.getValue().get(2).getMessage(), containsString("test3"));
    }

    @Test
    public void testInvalidOptionalPropertiesDoNotBreakAppender() throws InterruptedException {
        Layout<? extends Serializable> layout = null;
        String domain = "domain";
        String flushPeriodSecondsAttribute = "invalid";
        String messageTtlMsAttribute = "invalid";
        String discoveryTtlMsAttribute = "invalid";
        Filter filter = null;
        String ignoreExceptionsAttribute = "invalid";
        DistributedLoggingAppender appender = DistributedLoggingAppender.createAppender(name.getMethodName(),
                                                                                        layout,
                                                                                        domain,
                                                                                        flushPeriodSecondsAttribute,
                                                                                        messageTtlMsAttribute,
                                                                                        discoveryTtlMsAttribute,
                                                                                        filter,
                                                                                        ignoreExceptionsAttribute);

        appender.start();
        logger.addAppender(appender);
        logger.setAdditive(false);
        logger.setLevel(Level.DEBUG);

        logger.debug("test1");
        logger.debug("test2");
        logger.debug("test3");

        @SuppressWarnings("unchecked")
        ArgumentCaptor<List<JoynrLogEvent>> argument = ArgumentCaptor.forClass((Class) List.class);
        verify(loggingProxy, times(3)).log(argument.capture());

        List<List<JoynrLogEvent>> capturedLoggingEvents = argument.getAllValues();
        assertThat(capturedLoggingEvents.get(0).get(0).getMessage(), containsString("test1"));
        assertThat(capturedLoggingEvents.get(1).get(0).getMessage(), containsString("test2"));
        assertThat(capturedLoggingEvents.get(2).get(0).getMessage(), containsString("test3"));

    }

    @Test
    public void testInvalidNameReturnNullAppender() throws InterruptedException {
        Layout<? extends Serializable> layout = null;
        String appenderName = "";
        String domain = "domain";
        String flushPeriodSecondsAttribute = "1";
        String messageTtlMsAttribute = "1000";
        String discoveryTtlMsAttribute = "1000";
        Filter filter = null;
        String ignoreExceptionsAttribute = "true";
        DistributedLoggingAppender appender = DistributedLoggingAppender.createAppender(appenderName,
                                                                                        layout,
                                                                                        domain,
                                                                                        flushPeriodSecondsAttribute,
                                                                                        messageTtlMsAttribute,
                                                                                        discoveryTtlMsAttribute,
                                                                                        filter,
                                                                                        ignoreExceptionsAttribute);
        assertNull(appender);
    }

    @Test
    public void testNullNameReturnNullAppender() throws InterruptedException {
        Layout<? extends Serializable> layout = null;
        String appenderName = null;
        String domain = "domain";
        String flushPeriodSecondsAttribute = "1";
        String messageTtlMsAttribute = "1000";
        String discoveryTtlMsAttribute = "1000";
        Filter filter = null;
        String ignoreExceptionsAttribute = "true";
        DistributedLoggingAppender appender = DistributedLoggingAppender.createAppender(appenderName,
                                                                                        layout,
                                                                                        domain,
                                                                                        flushPeriodSecondsAttribute,
                                                                                        messageTtlMsAttribute,
                                                                                        discoveryTtlMsAttribute,
                                                                                        filter,
                                                                                        ignoreExceptionsAttribute);
        assertNull(appender);
    }

    @Test
    public void testInvalidDomainReturnNullAppender() throws InterruptedException {
        Layout<? extends Serializable> layout = null;
        String appenderName = "name";
        String domain = "";
        String flushPeriodSecondsAttribute = "1";
        String messageTtlMsAttribute = "1000";
        String discoveryTtlMsAttribute = "1000";
        Filter filter = null;
        String ignoreExceptionsAttribute = "true";
        DistributedLoggingAppender appender = DistributedLoggingAppender.createAppender(appenderName,
                                                                                        layout,
                                                                                        domain,
                                                                                        flushPeriodSecondsAttribute,
                                                                                        messageTtlMsAttribute,
                                                                                        discoveryTtlMsAttribute,
                                                                                        filter,
                                                                                        ignoreExceptionsAttribute);
        assertNull(appender);
    }

    @Test
    public void testNullDomainReturnNullAppender() throws InterruptedException {
        Layout<? extends Serializable> layout = null;
        String appenderName = "name";
        String domain = null;
        String flushPeriodSecondsAttribute = "1";
        String messageTtlMsAttribute = "1000";
        String discoveryTtlMsAttribute = "1000";
        Filter filter = null;
        String ignoreExceptionsAttribute = "true";
        DistributedLoggingAppender appender = DistributedLoggingAppender.createAppender(appenderName,
                                                                                        layout,
                                                                                        domain,
                                                                                        flushPeriodSecondsAttribute,
                                                                                        messageTtlMsAttribute,
                                                                                        discoveryTtlMsAttribute,
                                                                                        filter,
                                                                                        ignoreExceptionsAttribute);
        assertNull(appender);
    }

    @Test
    public void testAppenderWaitsForFlush() throws InterruptedException {
        Layout<? extends Serializable> layout = null;
        String domain = "domain";
        String flushPeriodSecondsAttribute = "1";
        String messageTtlMsAttribute = "1000";
        String discoveryTtlMsAttribute = "1000";
        Filter filter = null;
        String ignoreExceptionsAttribute = "true";
        DistributedLoggingAppender appender = DistributedLoggingAppender.createAppender(name.getMethodName(),
                                                                                        layout,
                                                                                        domain,
                                                                                        flushPeriodSecondsAttribute,
                                                                                        messageTtlMsAttribute,
                                                                                        discoveryTtlMsAttribute,
                                                                                        filter,
                                                                                        ignoreExceptionsAttribute);

        appender.start();
        logger.addAppender(appender);
        logger.setAdditive(false);
        logger.setLevel(Level.DEBUG);

        logger.debug("test1");
        Thread.sleep(1010);

        logger.debug("test2");
        Thread.sleep(1010);

        logger.debug("test3");
        Thread.sleep(1010);

        @SuppressWarnings("unchecked")
        ArgumentCaptor<List<JoynrLogEvent>> argument = ArgumentCaptor.forClass((Class) List.class);
        verify(loggingProxy, times(3)).log(argument.capture());

        List<List<JoynrLogEvent>> capturedLoggingEvents = argument.getAllValues();
        assertThat(capturedLoggingEvents.get(0).get(0).getMessage(), containsString("test1"));
        assertThat(capturedLoggingEvents.get(1).get(0).getMessage(), containsString("test2"));
        assertThat(capturedLoggingEvents.get(2).get(0).getMessage(), containsString("test3"));
    }

}
