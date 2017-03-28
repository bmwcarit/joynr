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

import io.joynr.arbitration.ArbitrationStrategy;
import io.joynr.arbitration.DiscoveryQos;
import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.ProxyBuilder;
import io.joynr.runtime.JoynrRuntime;

import java.util.LinkedList;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.atomic.AtomicInteger;

import joynr.system.JoynrLogEvent;
import joynr.system.LoggingProxy;

import org.apache.logging.log4j.core.appender.AbstractManager;

public class DistributedLoggingManager extends AbstractManager {

    private AtomicInteger appenderCount = new AtomicInteger(0);
    private LoggingProxy loggingProxy;
    private ConcurrentLinkedQueue<JoynrLogEvent> logEvents = new ConcurrentLinkedQueue<JoynrLogEvent>();
    private final Timer timer;
    private int flushPeriodSeconds;

    protected DistributedLoggingManager(JoynrRuntime runtime,
                                        String name,
                                        String domain,
                                        int flushPeriodSeconds,
                                        long messageTtlMs,
                                        long discoveryTtlMs) {
        super(name);
        this.flushPeriodSeconds = flushPeriodSeconds;

        DiscoveryQos discoveryQos = new DiscoveryQos();
        discoveryQos.setDiscoveryTimeoutMs(discoveryTtlMs);
        discoveryQos.setCacheMaxAgeMs(Long.MAX_VALUE);
        discoveryQos.setArbitrationStrategy(ArbitrationStrategy.HighestPriority);

        ProxyBuilder<LoggingProxy> proxyBuilder = runtime.getProxyBuilder(domain, LoggingProxy.class);

        loggingProxy = proxyBuilder.setMessagingQos(new MessagingQos(messageTtlMs))
                                   .setDiscoveryQos(discoveryQos)
                                   .build();

        timer = new Timer();

    }

    public void queue(JoynrLogEvent logEvent) {
        if (logEvent != null) {
            logEvents.add(logEvent);
        }

        if (flushPeriodSeconds <= 0) {
            flush();
        }
    }

    private void flush() {
        List<JoynrLogEvent> sendingEvents = new LinkedList<JoynrLogEvent>();

        JoynrLogEvent logEvent = logEvents.poll();
        while (logEvent != null) {
            sendingEvents.add(logEvent);
            logEvent = logEvents.poll();
        }

        if (sendingEvents.size() > 0) {
            loggingProxy.log(sendingEvents.toArray(new JoynrLogEvent[0]));
        }
    }

    public void appenderStarted() {
        int count = appenderCount.addAndGet(1);
        if (count == 1 && flushPeriodSeconds > 0) {
            TimerTask timerTask = new TimerTask() {
                @Override
                public void run() {
                    flush();
                }
            };

            timer.schedule(timerTask, flushPeriodSeconds * 1000, flushPeriodSeconds * 1000);
        }

    }

    public void appenderStopped() {
        int count = appenderCount.addAndGet(-1);
        if (count == 0) {
            timer.cancel();
        }
    }

}
