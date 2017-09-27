/*jslint node: true */

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
var LoggingManager = require('./LoggingManager');
var DefaultLoggerConfiguration = require('./DefaultLoggerConfiguration');
module.exports = (function(LoggingManager, defaultConfig) {
    /**
     * @name LoggerFactory
     * @class
     * @classdesc Global factory to create logger instances
     */
    var LoggerFactory = {};
    var loggingManager;
    LoggerFactory.init = function init(newLoggingManager) {
        loggingManager = newLoggingManager;
    };

    /**
     * @name LoggerFactory#getLogger
     * @function
     * @param {Object} name - The logger's name
     */
    LoggerFactory.getLogger = function getLogger(name) {
        if (loggingManager === undefined) {
            var newLoggingManager = new LoggingManager();
            newLoggingManager.configure(defaultConfig);
            LoggerFactory.init(newLoggingManager);
        }

        return loggingManager.getLogger(name);
    };

    return LoggerFactory;
}(LoggingManager, DefaultLoggerConfiguration));