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

define("joynr/capabilities/CapabilitiesRegistrar", [
    "global/Promise",
    "joynr/util/UtilInternal",
    "joynr/types/DiscoveryEntry",
    "joynr/types/CapabilityInformation",
    "joynr/capabilities/ParticipantIdStorage",
    "joynr/types/Version"
], function(Promise, Util, DiscoveryEntry, CapabilityInformation, ParticipantIdStorage, Version) {

    /**
     * The Capabilities Registrar
     *
     * @constructor
     * @name CapabilitiesRegistrar
     *
     * @param {Object}
     *            dependencies
     * @param {CapabilityDiscovery}
     *            dependencies.discoveryStub connects the inProcessStub to its skeleton
     * @param {MessageRouter}
     *            dependencies.messageRouter used to register nextHop for registered provider
     * @param {ParticipantIdStorage}
     *            dependencies.participantIdStorage connects the inProcessStub to its skeleton
     * @param {Address}
     *            dependencies.libjoynrMessagingAddress address to be used by the cluster controller to send incoming requests to this
     *            libjoynr's providers
     * @param {RequestReplyManager}
     *            dependencies.requestReplyManager passed on to providerAttribute, providerOperation and providerEvent
     * @param {PublicationManager}
     *            dependencies.publicationManager passed on to providerAttribute
     * @param {String}
     *            dependencies.localChannelId passed on to providerAttribute
     * 
     */
    function CapabilitiesRegistrar(dependencies) {
        var discoveryStub = dependencies.discoveryStub;
        var messageRouter = dependencies.messageRouter;
        var participantIdStorage = dependencies.participantIdStorage;
        var libjoynrMessagingAddress = dependencies.libjoynrMessagingAddress;
        var requestReplyManager = dependencies.requestReplyManager;
        var publicationManager = dependencies.publicationManager;
        var localChannelId = dependencies.localChannelId;
        var loggingManager = dependencies.loggingManager;

        /**
         * @param provider
         *            to scan for a ProviderOperation
         * @returns {Boolean} true if provider contains a ProviderOperation
         */
        function hasOperation(provider) {
            var property;
            for (property in provider) {
                if (provider.hasOwnProperty(property)) {
                    if (provider[property].constructor.name === "ProviderOperation") {
                        return true;
                    }
                }
            }
            return false;
        }

        /**
         * Registers a provider so that it is publicly available
         *
         * @function
         * @name CapabilitiesRegistrar#registerCapability
         * @deprecated registerCapability will be removed by 01.01.2017. Please use registerProvider instead.
         * NOTE: authToken is now ignored.
         */
        this.registerCapability = function registerCapability() {
            // remove first argument (authToken) before passing on to registerProvider
            Array.prototype.shift.apply(arguments);
            return this.registerProvider.apply(this, arguments);
        };

        /**
         * Registers a provider so that it is publicly available
         *
         * @function
         * @name CapabilitiesRegistrar#registerProvider
         *
         * @param {String}
         *            domain
         * @param {Object}
         *            provider
         * @param {String}
         *            provider.interfaceName
         * @param {ProviderQos}
         *            providerQos the Quality of Service parameters for provider registration
         * @param {Object}
         *            loggingContext optional logging context will be appended to logging messages created in the name of this proxy
         *
         * @returns {Object} an A+ promise
         */
        this.registerProvider =
                function registerProvider(domain, provider, providerQos, loggingContext) {

                    var missingImplementations = provider.checkImplementation();

                    if (missingImplementations.length > 0) {
                        throw new Error("provider: "
                            + domain
                            + "/"
                            + provider.interfaceName
                            + " is missing: "
                            + missingImplementations.toString());
                    }

                    // retrieve participantId
                    var participantId = participantIdStorage.getParticipantId(domain, provider);

                    if (loggingContext !== undefined) {
                        loggingManager.setLoggingContext(participantId, loggingContext);
                    }

                    // register provider at RequestReplyManager
                    requestReplyManager.addRequestCaller(participantId, provider);

                    // register routing address at routingTable
                    var messageRouterPromise =
                            messageRouter.addNextHop(participantId, libjoynrMessagingAddress);

                    // if provider has at least one attribute, add it as publication provider
                    publicationManager.addPublicationProvider(participantId, provider);

                    var discoveryStubPromise = discoveryStub.add(new DiscoveryEntry({
                        providerVersion : new Version(),
                        domain : domain,
                        interfaceName : provider.interfaceName,
                        participantId : participantId,
                        qos : providerQos,
                        lastSeenDateMs : Date.now()
                    }));

                    return Promise.all([
                        messageRouterPromise,
                        discoveryStubPromise
                    ]);
                };

        /**
         * Unregisters a provider so that it is not publicly available anymore
         *
         * @function
         * @name CapabilitiesRegistrar#unregisterCapability
         * @deprecated unregisterCapability will be removed by 01.01.2017. Please use unregisterProvider instead.
         * NOTE: authToken is now ignored.
         */
        this.unregisterCapability = function unregisterCapability() {
            // remove first argument (authToken) before passing on to unregisterProvider
            Array.prototype.shift.apply(arguments);
            return this.unregisterProvider.apply(this, arguments);
        };

        /**
         * Unregisters a provider so that it is not publicly available anymore
         *
         * @function
         * @name CapabilitiesRegistrar#unregisterProvider
         * @param {String}
         *            domain
         * @param {Object}
         *            provider
         * @param {String}
         *            provider.interfaceName
         * @returns {Object} an A+ promise
         */
        this.unregisterProvider = function unregisterProvider(domain, provider) {
            // retrieve participantId
            var participantId = participantIdStorage.getParticipantId(domain, provider);

            var discoveryStubPromise = discoveryStub.remove(participantId);

            // if provider has at least one attribute, remove it as publication
            // provider
            publicationManager.removePublicationProvider(participantId, provider);

            // unregister routing address at routingTable
            var messageRouterPromise = messageRouter.removeNextHop(participantId);

            // unregister provider at RequestReplyManager
            requestReplyManager.removeRequestCaller(participantId);

            return Promise.all([
                discoveryStubPromise,
                messageRouterPromise
            ]);
        };

    }

    return CapabilitiesRegistrar;

});
