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

    var jstestdriver = null;
    var JstdConsoleAppender = function JstdConsoleAppender() {};

    JstdConsoleAppender.prototype.append = function(loggingEvent) {
        var appender = this;

        var getFormattedMessage = function() {
            var layout = appender.getLayout();
            var formattedMessage = layout.format(loggingEvent);
            if (layout.ignoresThrowable() && loggingEvent.exception) {
                formattedMessage += loggingEvent.getThrowableStrRep();
            }
            return formattedMessage;
        };

        var formattedMessage = getFormattedMessage();
        try {
            jstestdriver.console.log(formattedMessage);
        } catch (e) {
            if (console) {
                console.log("ERROR LOGGING TO JSTD APPENDER: " + formattedMessage);
            }
        }

    };

    JstdConsoleAppender.prototype.group = function(name) {
        if (window && window.console && window.console.group) {
            window.console.group(name);
        }
    };

    JstdConsoleAppender.prototype.groupEnd = function() {
        if (window && window.console && window.console.groupEnd) {
            window.console.groupEnd();
        }
    };

    JstdConsoleAppender.prototype.toString = function() {
        return "JstdConsoleAppender";
    };

    // AMD support
    if (typeof define === 'function' && define.amd) {
        define("JstdConsoleAppender", [ "global/jstestdriver"
        ], function(jstestdriverDep) {
            jstestdriver = jstestdriverDep;
            return JstdConsoleAppender;
        });
    } else if (window) {
        window.JstdConsoleAppender = JstdConsoleAppender;
        jstestdriver = window.jstestdriver;
    }