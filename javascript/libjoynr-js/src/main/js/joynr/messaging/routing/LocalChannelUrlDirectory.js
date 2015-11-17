/*jslint es5: true */

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

define(
        "joynr/messaging/routing/LocalChannelUrlDirectory",
        [
            "global/Promise",
            "joynr/util/UtilInternal"
        ],
        function(Promise, Util) {

            /**
             * The LocalChannelUrlDirectory resolves channel IDs to channel URLs and maintains a local list as well as registering with the global
             * list. Channel IDs can be mapped on multiple channel URLs depending on network topology and reach-ability.
             *
             * @name LocalChannelUrlDirectory
             * @constructor
             *
             * @param {Object}
             *            settings used to configure LocalChannelUrlDirectory
             * @param {ChannelUrlDirectoryProxy}
             *            settings.channelUrlDirectoryProxy handles communication with the global channelurlDirectory
             * @param {Object}
             *            [settings.provisionedChannelUrls] preprovisioned channel URLs
             * @param {Array}
             *            settings.provisionedChannelUrls.CHANNELID a list of channel urls
             * @param {String}
             *            settings.provisionedChannelUrls.CHANNELID.array a string containing a Channel URL
             *
             * @classdesc The <code>LocalChannelUrlDirectory</code> maps known ChannelIds to ChannelUrls. Unknown channelIds are looked up in the Global
             * ChannelUrlDirectory
             */
            var LocalChannelUrlDirectory =
                    function(settings) {
                        var channelUrlDirectoryProxy, channelIdtoChannelUrlsMap, promiseLookups =
                                {};

                        Util.checkProperty(settings, "Object", "settings");
                        Util.checkPropertyIfDefined(
                                settings.provisionedChannelUrls,
                                "Object",
                                "settings.provisionedChannelUrls");

                        channelUrlDirectoryProxy = settings.channelUrlDirectoryProxy;
                        channelIdtoChannelUrlsMap =
                                Util.extend({}, settings.provisionedChannelUrls);

                        /**
                         * @name LocalChannelUrlDirectory#isStale
                         * @function
                         * @private
                         *
                         * @param {Number}
                         *            lastUpdated
                         * @param {Number}
                         *            freshness_ms
                         * @returns {Boolean} if the entry is stale
                         */
                        function isStale(lastUpdated, freshness_ms) {
                            // if lastUpdated is not set, then always considered to be fresh
                            return Date.now() - lastUpdated > freshness_ms;
                        }

                        /**
                         * Registers channelId and channelUrl information. The channel URL information is used to send messages to a Cluster Controller
                         * identified by the unique channel ID.
                         *
                         * @name LocalChannelUrlDirectory#registerChannelUrls
                         * @function
                         *
                         * @param {Object}
                         *            operationArguments - object containing properties required for the operation
                         * @param {Object}
                         *            operationArguments.channelId - the channel ID to register channel URL information with
                         * @param {ChannelUrlInformation}
                         *            operationArguments.channelUrlInformation - the channel URL information used to send messages to a Cluster Controller
                         * @returns {Object} A+ promise object
                         */
                        this.registerChannelUrls =
                                function registerChannelUrls(operationArguments) {
                                    // lastUpdated not set because this is a local entry with no
                                    // need to query global for freshness
                                    channelIdtoChannelUrlsMap[operationArguments.channelId] =
                                            operationArguments.channelUrlInformation;
                                    return channelUrlDirectoryProxy
                                            .registerChannelUrls(operationArguments);
                                };

                        /**
                         * Causes the channelUrls to be removed from the local cache and unregistered on the Global ChannelUrlDirectory
                         *
                         * @name LocalChannelUrlDirectory#unregisterChannelUrls
                         * @function
                         *
                         * @param {Object}
                         *            operationArguments - object containing properties required for the operation
                         * @param {Object}
                         *            operationArguments.channelId - the channel ID to register channel URL information with
                         * @returns {Object} A+ promise object
                         */
                        this.unregisterChannelUrls =
                                function unregisterChannelUrls(operationArguments) {
                                    delete channelIdtoChannelUrlsMap[operationArguments.channelId];
                                    return channelUrlDirectoryProxy
                                            .unregisterChannelUrls(operationArguments);
                                };

                        /**
                         * @name LocalChannelUrlDirectory#getUrlsForChannel
                         * @function
                         *
                         * @param {Object}
                         *            operationArguments - object containing properties required for the operation
                         * @param {Object}
                         *            operationArguments.channelId - the channel ID to register channel URL information with
                         * @param {Number}
                         *            freshness_ms
                         * @returns {Object} A+ promise object
                         */
                        this.getUrlsForChannel =
                                function getUrlsForChannel(operationArguments, freshness_ms) {
                                    var channelId, channelUrls, promise;
                                    channelId = operationArguments.channelId;
                                    channelUrls = channelIdtoChannelUrlsMap[channelId];

                                    promise = promiseLookups[channelId];
                                    if (promise !== undefined) {
                                        return promise;
                                    }

                                    // if missing or stale, lookup from global
                                    if ((channelUrls === undefined || isStale(channelUrls.lastUpdated, freshness_ms))) {
                                        promiseLookups[channelId] = channelUrlDirectoryProxy.getUrlsForChannel(operationArguments).then(
                                            function(opArgs) {
                                                if (opArgs.result === undefined) {
                                                    delete promiseLookups[channelId];
                                                    throw new Error("Could not retrieve URLs for channel "
                                                                + operationArguments.channelId
                                                                + ": resolved channelUrls are not defined.");
                                                } else {
                                                    var channelUrlWithDate =
                                                            Util.extend(opArgs.result,
                                                                        {
                                                                            lastUpdated : Date.now()
                                                                        });
                                                    channelIdtoChannelUrlsMap[operationArguments.channelId] =
                                                            channelUrlWithDate;
                                                    delete promiseLookups[channelId];
                                                    return channelIdtoChannelUrlsMap[operationArguments.channelId];
                                                }
                                            }).catch(function(error) {
                                                delete promiseLookups[channelId];
                                                throw new Error(
                                                        "Could not retrieve URLs for channel "
                                                            + operationArguments.channelId
                                                            + ": "
                                                            + error);
                                            });
                                        return promiseLookups[channelId];
                                    }
                                    return Promise.resolve(channelUrls);
                                };
                    };

            return LocalChannelUrlDirectory;
        });