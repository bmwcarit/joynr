/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

/*
 * in nodejs we don't have log4javascript but only log4js. Try to mimic the expected behavior of
 * LoggingManager with log4js.
 * //TODO: think about replacing log4javascript with log4js (for in browser execution)
 */
define("joynr/system/LoggingManager", [ "log4js"
], function(log4js) {
    /**
     * @name LoggingManager
     * @class
     * @variation 2
     *
     * @classdesc In nodejs we don't have log4javascript but only log4js.
     * Try to mimic the expected behavior of LoggingManager with log4js.
     */
    function LoggingManager() {
        /**
         * @function LoggingManager(2)#getLogger
         * @param {String} name - the name of the logger
         *
         * @returns {Object} the logger (creates a new one if it did not previously exist)
         */
        this.getLogger = function getLogger(name) {
            return log4js.getLogger(name);
        };

        /**
         * in node, always the default appender is used
         * @function LoggingManager(2)#registerAppenderClass
         */
        this.registerAppenderClass = function() {};

        /**
         * @function LoggingManager(2)#configure
         *
         * @param {Config} settings
         */
        this.configure = function configure(settings) {
            log4js.setGlobalLogLevel(settings.configuration.loggers.root.level.toUpperCase());
        };
    }

    LoggingManager.Appender = function Appender() {
        return {};
    };

    return LoggingManager;
});
