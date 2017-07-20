package io.joynr.logging;

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

import java.io.Serializable;

import joynr.system.JoynrLogEvent;
import joynr.system.JoynrLogLevel;
import joynr.system.JoynrLoggedError;
import joynr.system.JoynrLoggingContextTag;

import org.apache.commons.lang.math.NumberUtils;
import org.apache.logging.log4j.core.Filter;
import org.apache.logging.log4j.core.Layout;
import org.apache.logging.log4j.core.LogEvent;
import org.apache.logging.log4j.core.appender.AbstractAppender;
import org.apache.logging.log4j.core.appender.AppenderLoggingException;
import org.apache.logging.log4j.core.appender.ManagerFactory;
import org.apache.logging.log4j.core.config.plugins.Plugin;
import org.apache.logging.log4j.core.config.plugins.PluginAttribute;
import org.apache.logging.log4j.core.config.plugins.PluginElement;
import org.apache.logging.log4j.core.config.plugins.PluginFactory;
import org.apache.logging.log4j.core.helpers.Booleans;
import org.apache.logging.log4j.core.layout.PatternLayout;

@Plugin(name = "JoynrDistributed", elementType = "appender", category = "Core")
public class DistributedLoggingAppender extends AbstractAppender {

    private static DistributedLoggingManagerFactory factory = new DistributedLoggingManagerFactory();
    private final DistributedLoggingManager manager;

    private DistributedLoggingAppender(final String name,
                                       final Filter filter,
                                       final Layout<? extends Serializable> layout,
                                       final DistributedLoggingManager manager,
                                       final boolean ignoreExceptions) {
        super(name, filter, layout, ignoreExceptions);
        this.manager = manager;
    }

    /**
     * Pass of the logEvent to the manager, where it is written to the proxy
     *
     * @param event
     *            The LogEvent.
     */
    @Override
    public void append(final LogEvent event) {
        try {
            String message = new String(getLayout().toByteArray(event), "UTF-8");
            Throwable thrown = event.getThrown();
            JoynrLoggedError exception = null;
            if (thrown != null) {
                exception = new JoynrLoggedError(thrown.getClass().getName(), thrown.getMessage());
            }
            JoynrLogEvent logEvent = new JoynrLogEvent(event.getMillis(),
                                                       "1",
                                                       "host",
                                                       event.getFQCN(),
                                                       message,
                                                       JoynrLogLevel.valueOf(event.getLevel().name()),
                                                       exception,
                                                       new JoynrLoggingContextTag[0]);
            manager.queue(logEvent);
        } catch (final Exception ex) {
            throw new AppenderLoggingException(ex);
        }
    }

    @PluginFactory
    /**
     * @param name  (required) the name, used as an ID to retrieve the appender.
     * @param domain (required) The domain used by the joynr logging provider
     * @param layout (optional) The formatting layout to use. Must not be NULL
     * @param messageTtlMs (optional) time that logging event messages should live in transit before being discarded
     * @param discoveryTtlMs (optional) time that the proxy will look for a logging provider
     * @param filter (optional) The Filter or NULL.
     * @param ignoreExceptions
     * @return
     */
    // CHECKSTYLE IGNORE ParameterNumber FOR NEXT 1 LINES
    public static DistributedLoggingAppender createAppender(@PluginAttribute("name") final String name,
                                                            @PluginElement("Layout") Layout<? extends Serializable> layout,
                                                            @PluginAttribute("domain") final String domain,
                                                            @PluginAttribute("flushPeriodSeconds") final String flushPeriodSecondsAttribute,
                                                            @PluginAttribute("messageTtlMs") final String messageTtlMsAttribute,
                                                            @PluginAttribute("discoveryTtlMs") final String discoveryTtlMsAttribute,
                                                            @PluginElement("filters") final Filter filter,
                                                            @PluginAttribute("ignoreExceptions") final String ignoreExceptionsAttribute) {

        if (name == null || name.isEmpty()) {
            LOGGER.error("A name is required for DistributedLoggingAppender");
            return null;
        }
        if (domain == null || domain.isEmpty()) {
            LOGGER.error("A domain is required for DistributedLoggingAppender");
            return null;
        }
        if (layout == null) {
            layout = PatternLayout.createLayout(null, null, null, null, null, null);
        }

        boolean ignoreExceptions = Booleans.parseBoolean(ignoreExceptionsAttribute, false);
        long messageTtlMs = NumberUtils.toLong(messageTtlMsAttribute, Long.MAX_VALUE);
        long discoveryTtlMs = NumberUtils.toLong(discoveryTtlMsAttribute, Long.MAX_VALUE);
        int flushPeriodSeconds = NumberUtils.toInt(flushPeriodSecondsAttribute);

        DistributedLoggingManager manager = DistributedLoggingManager.getManager(name,
                                                                                 factory,
                                                                                 new FactoryData(domain,
                                                                                                 flushPeriodSeconds,
                                                                                                 messageTtlMs,
                                                                                                 discoveryTtlMs));
        return new DistributedLoggingAppender(name, filter, layout, manager, ignoreExceptions);
    }

    @Override
    public void start() {
        super.start();
        manager.appenderStarted();
    }

    @Override
    public void stop() {
        super.stop();
        manager.appenderStopped();
    }

    /**
     * Data to pass to factory method.
     */
    private static class FactoryData {
        private final long messageTtlMs;
        private final long discoveryTtlMs;
        private final String domain;
        private final int flushPeriodSeconds;

        public FactoryData(final String domain,
                           final int flushPeriodSeconds,
                           final long messageTtlMs,
                           final long discoveryTtlMs) {
            this.domain = domain;
            this.flushPeriodSeconds = flushPeriodSeconds;
            this.messageTtlMs = messageTtlMs;
            this.discoveryTtlMs = discoveryTtlMs;
        }
    }

    protected static class DistributedLoggingManagerFactory extends JoynrAppenderManagerFactory implements
            ManagerFactory<DistributedLoggingManager, FactoryData> {

        @Override
        public DistributedLoggingManager createManager(String name, FactoryData data) {
            return new DistributedLoggingManager(runtime,
                                                 name,
                                                 data.domain,
                                                 data.flushPeriodSeconds,
                                                 data.messageTtlMs,
                                                 data.discoveryTtlMs);
        }

    }

}
