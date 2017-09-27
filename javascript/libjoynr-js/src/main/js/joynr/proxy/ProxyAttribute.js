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
var Promise = require('../../global/Promise');
var UtilInternal = require('../util/UtilInternal');
var Request = require('../dispatching/types/Request');
var MessagingQos = require('../messaging/MessagingQos');
var Typing = require('../util/Typing');
var TypeRegistrySingleton = require('../../joynr/types/TypeRegistrySingleton');
module.exports =
        (function(Promise, Util, Request, MessagingQos, Typing, TypeRegistrySingleton) {

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

            var asRead = (function() {

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
                function get(settings) {
                    // ensure settings variable holds a valid object and initialize
                    // deferred object
                    settings = settings || {};
                    var request = new Request({
                        methodName : "get" + Util.firstUpper(this.attributeName)
                    });
                    return this.executeRequest(request, settings);
                }

                return function() {
                    this.get = get;
                };
            }());

            var asWrite =
                    (function() {

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
                        function set(settings) {
                            // ensure settings variable holds a valid object and initialize deferred
                            // object
                            settings = settings || {};
                            var error = checkArgument(settings.value);
                            if (error) {
                                return Promise.reject(new Error("error setting attribute: "
                                    + this.attributeName
                                    + ": "
                                    + error.toString()));
                            }

                            var request = new Request({
                                methodName : "set" + Util.firstUpper(this.attributeName),
                                paramDatatypes : [ this.attributeType
                                ],
                                params : [ settings.value
                                ]
                            });
                            return this.executeRequest(request, settings);
                        }

                        return function() {
                            this.set = set;
                        };
                    }());

            var asNotify =
                    (function() {

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
                        function subscribe(requestSettings) {
                            // return promise to caller
                            return this.settings.dependencies.subscriptionManager
                                    .registerSubscription({
                                        proxyId : this.parent.proxyParticipantId,
                                        providerDiscoveryEntry : this.parent.providerDiscoveryEntry,
                                        attributeName : this.attributeName,
                                        attributeType : this.attributeType,
                                        qos : requestSettings.subscriptionQos,
                                        subscriptionId : requestSettings.subscriptionId,
                                        onReceive : requestSettings.onReceive,
                                        onError : requestSettings.onError,
                                        onSubscribed : requestSettings.onSubscribed
                                    });
                        }

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
                        function unsubscribe(requestSettings) {
                            // passed in (right-most) messagingQos have precedence; undefined values are
                            // ignored
                            var messagingQos =
                                    new MessagingQos(Util.extend(
                                            {},
                                            this.parent.messagingQos,
                                            this.settings.messagingQos,
                                            requestSettings.messagingQos));

                            // return promise to caller
                            return this.settings.dependencies.subscriptionManager
                                    .unregisterSubscription({
                                        messagingQos : messagingQos,
                                        subscriptionId : requestSettings.subscriptionId
                                    });
                        }

                        return function() {
                            this.subscribe = subscribe;
                            this.unsubscribe = unsubscribe;
                        };
                    }());

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

                this.parent = parent;
                this.settings = settings;
                this.attributeName = attributeName;
                this.attributeType = attributeType;

                // TODO: should we do the attributeCaps-matching more strictly, like
                // this: (NOTIFY)?(READWRITE|((READ|WRITE)ONLY))?
                if (attributeCaps.match(/READ/)) {
                    asRead.call(this);
                }
                if (attributeCaps.match(/WRITE/)) {
                    asWrite.call(this);
                }
                if (attributeCaps.match(/NOTIFY/)) {
                    asNotify.call(this);
                }

                var publicProxyAttribute = Util.forward({}, this);
                return Object.freeze(publicProxyAttribute);
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
            ProxyAttribute.prototype.executeRequest =
                    function(request, requestSettings) {
                        // passed in (right-most) messagingQos have precedence; undefined values are
                        // ignored
                        var messagingQos =
                                Util.extend(
                                        new MessagingQos(),
                                        this.parent.messagingQos,
                                        this.settings.messagingQos,
                                        requestSettings.messagingQos);

                        function sendRequestOnSuccess(response) {
                            return Typing.augmentTypes(
                                    response[0],
                                    typeRegistry,
                                    this.attributeType);
                        }

                        // return promise to caller
                        return this.settings.dependencies.requestReplyManager.sendRequest({
                            toDiscoveryEntry : this.parent.providerDiscoveryEntry,
                            from : this.parent.proxyParticipantId,
                            messagingQos : messagingQos,
                            request : request
                        }).then(sendRequestOnSuccess.bind(this));
                    };

            return ProxyAttribute;
        }(Promise, UtilInternal, Request, MessagingQos, Typing, TypeRegistrySingleton));
