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

define(
        "joynr/proxy/ProxyAttribute",
        [
            "global/Promise",
            "joynr/util/UtilInternal",
            "joynr/dispatching/types/Request",
            "joynr/messaging/MessagingQos",
            "joynr/util/Typing",
            "joynr/types/TypeRegistrySingleton"
        ],
        function(Promise, Util, Request, MessagingQos, Typing, TypeRegistrySingleton) {

            var typeRegistry = TypeRegistrySingleton.getInstance();

            function checkArgument(value) {
                if (!Util.checkNullUndefined(value)) {
                    /*jslint nomen: true*/
                    var Constructor = typeRegistry.getConstructor(value._typeName);
                    /*jslint nomen: false*/

                    try {
                        if (Constructor && Constructor.checkMembers) {
                            Constructor.checkMembers(value, Typing.checkPropertyIfDefined);
                        }
                    } catch (error) {
                        return error;
                    }
                }
            }

            /**
             * Constructor of ProxyAttribute object that is used in the generation of proxy objects
             *
             * @constructor
             * @name ProxyAttribute
             *
             * @param {Object}
             *            parent is the proxy object that contains this attribute
             * @param {String}
             *            parent.fromParticipantId of the proxy itself
             * @param {String}
             *            parent.toParticipantId of the provider being addressed
             * @param {Object}
             *            settings the settings object for this function call
             * @param {Object}
             *            settings.dependencies the dependencies object for this function call
             * @param {RequestReplyManager}
             *            settings.dependencies.requestReplyManager
             * @param {SubscriptionManager}
             *            settings.dependencies.subscriptionManager
             * @param {DiscoveryQos}
             *            settings.discoveryQos the Quality of Service parameters for arbitration
             * @param {MessagingQos}
             *            settings.messagingQos the Quality of Service parameters for messaging
             * @param {String}
             *            attributeName the name of the attribute
             * @param {String}
             *            attributeType the type of the attribute
             * @param {String}
             *            attributeCaps the capabilities of the attribute:
             *            [NOTIFY][READWRITE|READONLY|WRITEONLY], e.g. NOTIFYREADWRITE
             *            if the string == 'NOTIFY' this attribute has subscribe and unsubscribe
             *            if the string contains 'READ' or'WRITE' this attribute has the get and set
             */
            function ProxyAttribute(parent, settings, attributeName, attributeType, attributeCaps) {
                if (!(this instanceof ProxyAttribute)) {
                    // in case someone calls constructor without new keyword
                    // (e.g. var c = Constructor({..}))
                    return new ProxyAttribute(
                            parent,
                            settings,
                            attributeName,
                            attributeType,
                            attributeCaps);
                }

                /**
                 * @name ProxyAttribute#executeRequest
                 * @function
                 * @private
                 * @param {Request}
                 *            request
                 * @param {Object}
                 *            requestSettings
                 * @param {MessagingQos}
                 *            requestSettings.messagingQos
                 * @returns {Object} an A+ promise
                 */
                function executeRequest(request, requestSettings) {
                    // passed in (right-most) messagingQos have precedence; undefined values are
                    // ignored
                    var messagingQos =
                            Util.extend(
                                    new MessagingQos(),
                                    parent.messagingQos,
                                    settings.messagingQos,
                                    requestSettings.messagingQos);

                    function sendRequestOnSuccess(response) {
                        return Typing.augmentTypes(response[0], typeRegistry, attributeType);
                    }

                    // return promise to caller
                    return settings.dependencies.requestReplyManager.sendRequest({
                        toDiscoveryEntry : parent.providerDiscoveryEntry,
                        from : parent.proxyParticipantId,
                        messagingQos : messagingQos,
                        request : request
                    }).then(sendRequestOnSuccess);
                }

                /**
                 * @name ProxyAttribute#executeSubscriptionRequest
                 * @function
                 * @private
                 * @param {Object}
                 *            requestSettings
                 * @param {MessagingQos}
                 *            requestSettings.messagingQos
                 * @param {SubscriptionQos}
                 *            requestSettings.subscriptionQos
                 * @param {Function}
                 *            requestSettings.onReceive
                 * @param {Function}
                 *            requestSettings.onError
                 * @param {Function}
                 *            requestSettings.onSubscribed
                 * @param {String}
                 *            requestSettings.subscriptionId optional parameter subscriptionId to
                 *            reuse a preexisting identifier for this concrete subscription request
                 * @returns {Object} an A+ promise
                 */
                function executeSubscriptionRequest(requestSettings) {
                    // return promise to caller
                    return settings.dependencies.subscriptionManager.registerSubscription({
                        proxyId : parent.proxyParticipantId,
                        providerDiscoveryEntry : parent.providerDiscoveryEntry,
                        attributeName : attributeName,
                        attributeType : attributeType,
                        qos : requestSettings.subscriptionQos,
                        subscriptionId : requestSettings.subscriptionId,
                        onReceive : requestSettings.onReceive,
                        onError : requestSettings.onError,
                        onSubscribed : requestSettings.onSubscribed
                    });
                }

                /**
                 * @name ProxyAttribute#executeSubscriptionStop
                 * @function
                 * @private
                 * @param {Object}
                 *            requestSettings
                 * @param {MessagingQos}
                 *            requestSettings.messagingQos
                 * @param {String}
                 *            requestSettings.subscriptionId
                 * @returns {Object} an A+ promise
                 */
                function executeSubscriptionStop(requestSettings) {
                    // passed in (right-most) messagingQos have precedence; undefined values are
                    // ignored
                    var messagingQos =
                            new MessagingQos(Util.extend(
                                    {},
                                    parent.messagingQos,
                                    settings.messagingQos,
                                    requestSettings.messagingQos));

                    // return promise to caller
                    return settings.dependencies.subscriptionManager.unregisterSubscription({
                        messagingQos : messagingQos,
                        subscriptionId : requestSettings.subscriptionId
                    });

                }

                // TODO: should we do the attributeCaps-matching more strictly, like
                // this: (NOTIFY)?(READWRITE|((READ|WRITE)ONLY))?
                if (attributeCaps.match(/READ/)) {
                    /**
                     * Getter for attribute
                     *
                     * @name ProxyAttribute#get
                     * @function
                     *
                     * @param {Object}
                     *            [settings] the settings object for this function call
                     * @returns {Object} returns an A+ promise object that will alternatively accept
                     *            callback functions through its setters
                     *          "then(function ({?}value){..}).catch(function ({string}error){..})"
                     */
                    this.get = function get(settings) {
                        // ensure settings variable holds a valid object and initialize
                        // deferred object
                        settings = settings || {};
                        var request = new Request({
                            methodName : "get" + Util.firstUpper(attributeName)
                        });
                        return executeRequest(request, settings);
                    };
                }
                if (attributeCaps.match(/WRITE/)) {
                    /**
                     * Setter for attribute
                     *
                     * @name ProxyAttribute#set
                     * @function
                     *
                     * @param {Object}
                     *            settings the settings object for this function call
                     * @param {Object}
                     *            settings.value the attribute value to set
                     * @returns {Object} returns an A+ promise
                     */
                    this.set =
                            function set(settings) {
                                // ensure settings variable holds a valid object and initialize deferred
                                // object
                                settings = settings || {};
                                var error = checkArgument(settings.value);
                                if (error) {
                                    return Promise.reject(new Error("error setting attribute: "
                                        + attributeName
                                        + ": "
                                        + error.toString()));
                                }

                                var request = new Request({
                                    methodName : "set" + Util.firstUpper(attributeName),
                                    paramDatatypes : [ attributeType
                                    ],
                                    params : [ settings.value
                                    ]
                                });
                                return executeRequest(request, settings);
                            };
                }
                if (attributeCaps.match(/NOTIFY/)) {
                    /**
                     * Subscription to isOn attribute
                     *
                     * @name ProxyAttribute#subscribe
                     * @function
                     *
                     * @param {Object}
                     *            settings the settings object for this function call
                     *
                     * @param {SubscriptionQos}
                     *            settings.subscriptionQos the subscription quality of service object
                     * @param {MessagingQos}
                     *            settings.messagingQos The messagingQos to use for this
                     *            subscriptionRequest.
                     *
                     * @param {Function}
                     *            settings.onReceive this function is called if the attribute has
                     *            been published successfully, method signature:
                     *            "void onReceive({?}value)"
                     * @param {Function}
                     *            settings.onError this function is called if a publication of the
                     *            attribute value was missed, method signature: "void onError({Error} error)"
                     * @param {Function}
                     *            settings.onSubscribed the callback to inform once the subscription request has
                     *            been delivered successfully
                     * @param {String}
                     *            settings.subscriptionId optional subscriptionId to be used for the
                     *            new subscription
                     * @returns {Object} returns an A+ promise object
                     */
                    this.subscribe = function subscribe(settings) {
                        return executeSubscriptionRequest(settings);
                    };

                    /**
                     * Unsubscribe from the attribute
                     *
                     * @name ProxyAttribute#unsubscribe
                     * @function
                     *
                     * @param {Object}
                     *            settings the settings object for this function call
                     * @param {String}
                     *            settings.subscriptionId the subscription id retrieved from the
                     *            subscribe function
                     * @returns {Object} returns an A+ promise object that will alternatively accept
                     *            the callback functions through the its
                     *            functions "then(function()).catch(function ({string}error){..})"
                     * @throws {String}
                     *             if the subscription does not exist
                     * @see ProxyAttribute#subscribe
                     */
                    this.unsubscribe = function unsubscribe(settings) {
                        return executeSubscriptionStop(settings);
                    };
                }
            }

            return ProxyAttribute;
        });
