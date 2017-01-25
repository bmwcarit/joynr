/*global postMessage: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

define("joynr/system/DistributedLoggingAppender", [
    "joynr/system/JoynrLogEvent",
    "joynr/system/JoynrLoggingContextTag",
    "joynr/system/LoggingManager",
    "joynr/util/UtilInternal",
    "joynr/util/JSONSerializer"
], function(JoynrLogEvent, JoynrLoggingContextTag, LoggingManager, Util, JSONSerializer) {
    var DEFAULT_FLUSH_INTERVAL_MS = 60000;
    var DEFAULT_FLUSH_MAX_LOGEVENTS_COUNT = 20;
    /**
     * A log4javascript Appender that sends a logged message to a joyn logging provider,
     * where it can be combined with logs from other sources. Log events are first saved
     * locally, and then sent periodically or after a message threshold has been reached.
     *
     * @name DistributedLoggingAppender
     * @constructor
     *
     * @param config
     * @param loggingContexts
     */
    function DistributedLoggingAppender(config, loggingContexts) {
        var flushInterval, flushIntervalId, flushMaxLogEventsCount, queuedEvents;
        var loggingProxy = null;
        var eventsLostCount = 0;
        queuedEvents = [];
        config = config || {};
        flushInterval = Number(config.flushInterval) || DEFAULT_FLUSH_INTERVAL_MS;
        flushMaxLogEventsCount =
                Number(config.flushMaxLogEventsCount) || DEFAULT_FLUSH_MAX_LOGEVENTS_COUNT;

        function logLostEventsCount(count) {
            var logLostEventsEvent =
                    new JoynrLogEvent({
                        timestamp : Date.now(),
                        eventVersion : "1",
                        path : "joynr/system/logging/",
                        message : count
                            + " newer events were discarded because"
                            + " the logging queue was full.",
                        priority : "ERROR"
                    });
            loggingProxy.log(logLostEventsEvent);

        }

        /**
         * If the loggingProxy is available, log out the logged events, reset the event
         * queue and log if any events were lost
         */
        function flush() {
            if (loggingProxy === null) {
                return;
            }

            try {
                loggingProxy.log({
                    logEvents : queuedEvents
                });
                queuedEvents.length = 0;
                if (eventsLostCount > 0) {
                    logLostEventsCount(eventsLostCount);
                }
            } catch (error) {
                // nothing we can do here
            }
        }

        /**
         * returns the whole diagnostic tags object, or an empty object
         */
        function getDiagnosticTags(loggingEvent) {
            var i, message;
            for (i = 0; i < loggingEvent.messages.length; i++) {
                message = loggingEvent.messages[i];
                if (message.hasOwnProperty("diagnosticTag")) {
                    return message;
                }
            }
            return {};
        }

        /**
         * returns the named tag, or the empty string
         */
        function getDiagnosticTag(loggingEvent, tagName) {
            return getDiagnosticTags(loggingEvent)[tagName] || "";
        }

        /**
         * Says whether the message was directly created as a result of this appenders use
         * of the loggingProxy.
         */
        function isOwnLoggingEvent(loggingEvent) {
            var from;
            if (loggingProxy !== null) {
                from = getDiagnosticTag(loggingEvent, "from");
                if (from === loggingProxy.proxyParticipantId
                    || from === loggingProxy.providerDiscoveryEntry.participantId) {
                    return true;
                }
            }
            return false;
        }

        /**
         * Implementing the appender function of log4javascript appenders
         *
         * @name DistributedLoggingAppender#append
         * @function
         * @param loggingEvent
         */
        this.append = function append(loggingEvent) {
            var loggedEvent, from, senderLoggingContext, tags;
            if (isOwnLoggingEvent(loggingEvent)) {
                return;
            }

            from = getDiagnosticTag(loggingEvent, "from");
            senderLoggingContext = loggingContexts[from] || {};
            Util.extend(senderLoggingContext, getDiagnosticTags(loggingEvent));

            tags = Util.transform(senderLoggingContext, function(value, key) {
                var stringValue;
                if (typeof value === "object") {
                    stringValue = JSONSerializer.stringify(value);
                } else {
                    stringValue = value.toString();
                }

                return new JoynrLoggingContextTag({
                    key : key,
                    value : stringValue
                });
            });

            loggedEvent = new JoynrLogEvent({
                timestamp : loggingEvent.timeStampInMilliseconds,
                eventVersion : "1",
                path : loggingEvent.logger.name,
                message : JSONSerializer.stringify(loggingEvent.messages),
                priority : loggingEvent.level.name,
                tags : tags
            });
            queuedEvents.push(loggedEvent);

            if (queuedEvents.length >= flushMaxLogEventsCount) {
                flush();
                if (queuedEvents.length !== 0) {
                    queuedEvents.pop();
                    eventsLostCount++;
                }
            }
        };

        /**
         * called once the logging proxy has been arbitrated, this function starts the
         * interval timer that sends the logged events at set
         * intervals as set in the logging properties on the appender
         *
         * @name DistributedLoggingAppender#setProxy
         * @function
         * @param newProxy
         */
        this.setProxy = function setProxy(newProxy) {
            loggingProxy = newProxy;
            flushIntervalId = setInterval(flush, flushInterval);
        };

        this.toString = function toString() {
            return "DistributedLoggingAppender";
        };

        this.shutdown = function shutdown() {
            if (flushIntervalId !== undefined) {
                clearInterval(flushIntervalId);
            }
        };
    }

    return DistributedLoggingAppender;

});
