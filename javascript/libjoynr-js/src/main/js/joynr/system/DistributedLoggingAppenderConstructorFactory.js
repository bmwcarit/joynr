/*jslint es5: true, node: true, node: true */
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
var DistributedLoggingAppender = require('./DistributedLoggingAppender');
var LoggingProxy = require('./LoggingProxy');
var DiscoveryQos = require('../proxy/DiscoveryQos');
var DiscoveryScope = require('../../joynr/types/DiscoveryScope');
var LoggerFactory = require('./LoggerFactory');
var Util = require('../util/UtilInternal');

            /**
             * A Factory to create a DistributedLoggingAppender constructor that contains a closure
             * to register a loggingProxy once the proxy has been created
             *
             * @name DistributedLoggingAppenderConstructorFactory
             * @class
             */
            var DistributedLoggingAppenderConstructorFactory = {};

            /**
             * builds a DistributedLoggingAppender constructor that contains a closure to register a
             * loggingProxy once the proxy has been created
             * @function
             * @name DistributedLoggingAppenderConstructorFactory#build
             * @param {ProxyBuilder} proxyBuilder
             * @param {MessagingQos} messagingQos
             */
            DistributedLoggingAppenderConstructorFactory.build =
                    function build(proxyBuilder, messagingQos) {
                        var log =
                                LoggerFactory
                                        .getLogger("joynr.system.DistributedLoggingAppenderConstructorFactory");

                        return function(config, loggingContexts) {
                            var errorString, newAppender;

                            newAppender = new DistributedLoggingAppender(config, loggingContexts);

                            proxyBuilder.build(LoggingProxy, {
                                domain : "io.joynr",
                                messagingQos : messagingQos,
                                discoveryQos : new DiscoveryQos({
                                    discoveryScope : DiscoveryScope.GLOBAL_ONLY,
                                    cacheMaxAgeMs : Util.getMaxLongValue()
                                })
                            }).then(function(newLoggingProxy) {
                                newAppender.setProxy(newLoggingProxy);
                                return newLoggingProxy;
                            }).catch(function(error) {
                                errorString = "Failed to create proxy for logging: " + error;
                                log.debug(errorString);
                                return error;
                            });

                            return newAppender;
                        };
                    };

module.exports = DistributedLoggingAppenderConstructorFactory;