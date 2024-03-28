/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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
package io.joynr.discovery.pt.jee;

import static io.joynr.discovery.pt.jee.StandalonePTUtil.writeDataToCsvFile;
import static io.joynr.jeeintegration.api.JeeIntegrationPropertyKeys.JEE_MESSAGING_SCHEDULED_EXECUTOR_RESOURCE;

import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

import javax.annotation.PostConstruct;
import javax.annotation.PreDestroy;
import javax.annotation.Resource;
import javax.ejb.Singleton;
import javax.ejb.Startup;
import com.google.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.statusmetrics.JoynrStatusMetrics;

@Singleton
@Startup
public class StatusMetricsPrinter {
    private static final Logger logger = LoggerFactory.getLogger(StatusMetricsPrinter.class);

    private JoynrStatusMetrics statusMetrics;

    @Resource(name = JEE_MESSAGING_SCHEDULED_EXECUTOR_RESOURCE)
    private ScheduledExecutorService statusMetricsScheduler;

    private ScheduledFuture<?> scheduledFuture;
    private long printUpdateIntervalMs;
    private long initialReceivedMessages = 0;
    private long initialSentMessages = 0;
    private long receivedMessages = 0;
    private long sentMessages = 0;
    private String csvFile;

    @Inject
    public StatusMetricsPrinter(JoynrStatusMetrics statusMetrics) {
        logger.trace("###SMP CONSTRUCTOR called: " + this);
        this.statusMetrics = statusMetrics;
        this.csvFile = System.getenv("PT_RESULTS");
        this.printUpdateIntervalMs = Long.parseLong(System.getenv("PT_LOGGING_INTERVAL_MS"));
    }

    @PostConstruct
    public void postConstruct() {
        logger.trace("###SMP postConstruct: " + this);
        this.printStatusMetricsPeriodically();
    }

    @PreDestroy
    public void destroy() {
        scheduledFuture.cancel(false);
    }

    private void printStatusMetricsPeriodically() {
        logger.trace("Setting up periodic print update with interval {}", printUpdateIntervalMs);
        final Runnable command = new Runnable() {
            public void run() {
                try {
                    logger.debug("Write results each: {}", printUpdateIntervalMs);
                    printMetrics();
                    logger.debug("Write results each: {} DONE", printUpdateIntervalMs);
                } catch (Exception e) {
                    logger.error("Error in writting", e);
                }
            }
        };
        scheduledFuture = this.statusMetricsScheduler.scheduleAtFixedRate(command,
                                                                          printUpdateIntervalMs,
                                                                          printUpdateIntervalMs,
                                                                          TimeUnit.MILLISECONDS);
        logger.trace("### Setting up periodic print update with interval {} DONE", printUpdateIntervalMs);
    }

    public String printMetrics() {
        String result = statusMetrics.getAllConnectionStatusMetrics().stream().map(m -> {
            String metricsString = "\n*** ConnectionStatusMetrics***";
            this.receivedMessages = m.getReceivedMessages() - this.initialReceivedMessages;
            this.sentMessages = m.getSentMessages() - this.initialSentMessages;
            this.initialReceivedMessages = m.getReceivedMessages();
            this.initialSentMessages = m.getSentMessages();
            metricsString += "\nRECEIVED messages: " + this.receivedMessages;
            metricsString += "\nSENT messages: " + this.sentMessages;
            metricsString += "\n******************************";
            return metricsString;
        }).collect(Collectors.joining());
        logger.info(result);
        writeDataToCsvFile(csvFile, receivedMessages, sentMessages);
        this.receivedMessages = 0;
        this.sentMessages = 0;
        return result;
    }

}
