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

define("joynr/proxy/ProxyEvent", [ "joynr/proxy/BroadcastFilterParameters"
], function(BroadcastFilterParameters) {

    /**
     * Constructor of ProxyEvent object that is used in the generation of proxy objects
     *
     * @constructor
     * @name ProxyEvent
     * @param {Object}
     *            settings the settings for this broadcast proxy
     * @param {String}
     *            settings.fromParticipantId of the proxy itself
     * @param {String}
     *            settings.toParticipantId of the provider being addressed
     * @param {DiscoveryQos}
     *            settings.discoveryQos the Quality of Service parameters for arbitration
     * @param {MessagingQos}
     *            settings.messagingQos the Quality of Service parameters for messaging
     * @param {Object}
     *            settings.dependencies the dependencies object for this function call
     * @param {SubscriptionManager}
     *            settings.dependencies.subscriptionManager
     * @param {String}
     *            settings.broadcastName the name of the broadcast as modelled in Franca
     * @returns {ProxyEvent}
     */
    function ProxyEvent(parent, settings) {
        if (!(this instanceof ProxyEvent)) {
            // in case someone calls constructor without new keyword (e.g. var c =
            // Constructor({..}))
            return new ProxyEvent(parent, settings);
        }

        /**
         * @name ProxyEvent#subscribe
         * @function
         * @param {Object}
         *            subscribeParameters the settings object for this function call
         * @param {SubscriptionQos}
         *            subscribeParameters.subscriptionQos the subscription quality of service object
         * @param {String}
         *            [subscribeParameters.subscriptionId] optional subscriptionId. Used to refresh or
         *            reinstate an existing subscription.
         * @param {onReceive}
         *            subscribeParameters.onReceive this function is called when an event as been
         *            received. method signature: "void onReceive({?}value)"
         * @param {onError}
         *            subscribeParameters.onError this function is called when an error occurs with
         *            a subscribed event. method signature: "void onError({Error} error)"
         * @returns {Object} returns a promise that is resolved with the subscriptionId, which is to
         *          be used to unsubscribe from this subscription later. NOTE: currently resolved
         *          when the request is sent; later will be resolved once the subscriptionReply is
         *          received. See TODO # 1319
         */
        this.subscribe = function subscribe(subscribeParameters) {
            return settings.dependencies.subscriptionManager.registerBroadcastSubscription({
                proxyId : parent.proxyParticipantId,
                providerId : parent.providerParticipantId,
                messagingQos : settings.messagingQos,
                broadcastName : settings.broadcastName,
                subscriptionQos : subscribeParameters.subscriptionQos,
                subscriptionId : subscribeParameters.subscriptionId,
                onReceive : subscribeParameters.onReceive,
                onError : subscribeParameters.onError,
                filterParameters : subscribeParameters.filterParameters
            });
        };

        this.createFilterParameters = function createFilterParameters() {
            return new BroadcastFilterParameters(settings.filterParameters);
        };

        /**
         * @name ProxyEvent#unsubscribe
         * @function
         * @param {Object}
         *            unsubscribeParameters the settings object for this function call
         * @param {Object}
         *            unsubscribeParameters.subscriptionId the subscription token retrieved from the
         *            subscribe function
         * @returns {Object} returns a promise that is resolved when unsubscribe has been executed.
         * @see ProxyEvent#subscribe
         */
        this.unsubscribe = function unsubscribe(unsubscribeParameters) {
            return settings.dependencies.subscriptionManager.unregisterSubscription({
                messagingQos : settings.messagingQos,
                subscriptionId : unsubscribeParameters.subscriptionId
            });
        };

        return Object.freeze(this);
    }

    return ProxyEvent;

});
