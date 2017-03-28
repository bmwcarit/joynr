/*global console: true */

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

define("joynr/system/ConsoleAppender", [], function() {

    /**
     * A log4javascript Appender that sends a logged message from a WebWorker to the main context to
     * log it there
     *
     * @name ConsoleAppender
     * @constructor
     */
    function ConsoleAppender() {}

    /**
     * Implementing the appender function of log4javascript appenders
     *
     * @name ConsoleAppender#append
     * @function
     */
    ConsoleAppender.prototype.append = function(loggingEvent) {
        var formattedMessage = loggingEvent.getCombinedMessages(), appender = this;

        var getFormattedMessage = function() {
            try {
                var layout = appender.getLayout();
                formattedMessage = layout.format(loggingEvent);
                if (layout.ignoresThrowable() && loggingEvent.exception) {
                    formattedMessage += loggingEvent.getThrowableStrRep();
                }
            } catch (e) {

            }
            return formattedMessage;
        };

        var logLevel = loggingEvent.level.name.toLowerCase();
        formattedMessage = getFormattedMessage();
        console[logLevel] = console[logLevel] || console.log;
        console[logLevel](formattedMessage);
    };

    ConsoleAppender.prototype.toString = function toString() {
        return "ConsoleAppender";
    };

    return ConsoleAppender;

});