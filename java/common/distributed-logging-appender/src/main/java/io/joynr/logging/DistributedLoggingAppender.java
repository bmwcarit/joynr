package io.joynr.logging;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.List;

import joynr.system.JoynrLogEvent;
import joynr.system.JoynrLogLevel;
import joynr.system.JoynrLoggedException;
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
    private DistributedLoggingManager manager;

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
            JoynrLoggedException exception = null;
            if (thrown != null) {
                exception = new JoynrLoggedException(thrown.getClass().getName(), thrown.getMessage());
            }
            List<JoynrLoggingContextTag> tags = new ArrayList<JoynrLoggingContextTag>();
            JoynrLogEvent logEvent = new JoynrLogEvent(event.getMillis(),
                                                       "1",
                                                       "host",
                                                       event.getFQCN(),
                                                       message,
                                                       JoynrLogLevel.valueOf(event.getLevel().name()),
                                                       exception,
                                                       tags);
            manager.queue(logEvent);
        } catch (final Exception ex) {
            throw new AppenderLoggingException(ex);
        }
    }

    /**
     * 
     * @param name
     * @param layout
     *            (required) The formatting layout to use. Must not be NULL
     * @param filter
     *            (optional) The Filter or NULL.
     * @return an appender, or NULL if incorrectly called.
     */

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
        private String domain;
        private int flushPeriodSeconds;

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