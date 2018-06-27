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
const Promise = require("../../global/Promise");
const DiscoveryEntry = require("../../generated/joynr/types/DiscoveryEntry");
const ProviderScope = require("../../generated/joynr/types/ProviderScope");
const Version = require("../../generated/joynr/types/Version");
let defaultExpiryIntervalMs = 6 * 7 * 24 * 60 * 60 * 1000; // 6 Weeks
const loggingManager = require("../system/LoggingManager");
const log = loggingManager.getLogger("joynr.capabilities.CapabilitiesRegistrar");
const UtilInternal = require("../util/UtilInternal");

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
 *
 */
function CapabilitiesRegistrar(dependencies) {
    this._discoveryStub = dependencies.discoveryStub;
    this._messageRouter = dependencies.messageRouter;
    this._participantIdStorage = dependencies.participantIdStorage;
    this._libjoynrMessagingAddress = dependencies.libjoynrMessagingAddress;
    this._requestReplyManager = dependencies.requestReplyManager;
    this._publicationManager = dependencies.publicationManager;
    this._started = true;
}

/*
 * Internal function used to throw exception in case of CapabilitiesRegistrar
 * is not used properly
 */
CapabilitiesRegistrar.prototype._checkIfReady = function() {
    if (!this._started) {
        throw new Error("CapabilitiesRegistrar is already shut down");
    }
};

/**
 * Registers a provider so that it is publicly available
 *
 * @function
 * @name CapabilitiesRegistrar#registerProvider
 *
 * @param {Object} settings the arguments object for this function call
 * @param {String}
 *            settings.domain
 * @param {Object}
 *            settings.provider
 * @param {String}
 *            settings.provider.interfaceName
 * @param {ProviderQos}
 *            settings.providerQos the Quality of Service parameters for provider registration
 * @param {Number}
 *            [settings.expiryDateMs] date in millis since epoch after which the discovery entry can be purged from all directories.
 *            Default value is one day.
 * @param {Object}
 *            [settings.loggingContext] optional logging context will be appended to logging messages created in the name of this proxy
 * @param {String}
 *            [settings.participantId] optional. If not set, a globally unique UUID participantId will be generated, and persisted to
 *            localStorage. If set, the participantId must be unique in the context of the provider's scope, as set in the ProviderQos;
 *            The application setting the participantId is responsible for guaranteeing uniqueness.
 * @param {Boolean}
 *            [settings.awaitGlobalRegistration] optional. If provided and set to true registerProvider will wait until local and global
 *            registration succeeds or timeout is reached: otherwise registerProvider only waits for local registration.
 *
 * @returns {Object} an A+ promise
 */
CapabilitiesRegistrar.prototype.register = function register(settings) {
    return this.registerProvider(
        settings.domain,
        settings.provider,
        settings.providerQos,
        settings.expiryDateMs,
        settings.loggingContext,
        settings.participantId,
        settings.awaitGlobalRegistration
    );
};

/**
 * Registers a provider so that it is publicly available
 *
 * @deprecated Use register instead
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
 * @param {Number}
 *            [expiryDateMs] date in millis since epoch after which the discovery entry can be purged from all directories.
 *            Default value is one day.
 * @param {Object}
 *            [loggingContext] optional logging context will be appended to logging messages created in the name of this proxy
 * @param {String}
 *            [participantId] optional. If not set, a globally unique UUID participantId will be generated, and persisted to localStorage.
 * @param {Boolean}
 *            [awaitGlobalRegistration] optional. If provided and set to true registerProvider will wait until local and global
 *            registration succeeds or timeout is reached: otherwise registerProvider only waits for local registration.
 *
 * @returns {Object} an A+ promise
 */
CapabilitiesRegistrar.prototype.registerProvider = async function registerProvider(
    domain,
    provider,
    providerQos,
    expiryDateMs,
    loggingContext,
    participantId,
    awaitGlobalRegistration
) {
    this._checkIfReady();

    const missingImplementations = provider.checkImplementation();

    if (missingImplementations.length > 0) {
        throw new Error(
            `provider: ${domain}/${provider.interfaceName} is missing: ${missingImplementations.toString()}`
        );
    }

    // retrieve participantId if not passed in
    if (participantId === undefined || participantId === null) {
        participantId = this._participantIdStorage.getParticipantId(domain, provider);
    }

    if (loggingContext !== undefined) {
        log.warn("loggingContext is currently not supported");
    }

    if (awaitGlobalRegistration === undefined) {
        awaitGlobalRegistration = false;
    }

    if (typeof awaitGlobalRegistration !== "boolean") {
        const errText = "awaitGlobalRegistration must be boolean";
        log.warn(errText);
        return Promise.reject(new Error(errText));
    }

    // register provider at RequestReplyManager
    this._requestReplyManager.addRequestCaller(participantId, provider);

    // if provider has at least one attribute, add it as publication provider
    this._publicationManager.addPublicationProvider(participantId, provider);

    // register routing address at routingTable
    const isGloballyVisible = providerQos.scope === ProviderScope.GLOBAL;
    await this._messageRouter.addNextHop(participantId, this._libjoynrMessagingAddress, isGloballyVisible);

    // TODO: Must be later provided by the user or retrieved from somewhere
    const defaultPublicKeyId = "";

    try {
        await this._discoveryStub.add(
            new DiscoveryEntry({
                providerVersion: new Version({
                    majorVersion: provider.constructor.MAJOR_VERSION,
                    minorVersion: provider.constructor.MINOR_VERSION
                }),
                domain,
                interfaceName: provider.interfaceName,
                participantId,
                qos: providerQos,
                publicKeyId: defaultPublicKeyId,
                expiryDateMs: expiryDateMs || Date.now() + defaultExpiryIntervalMs,
                lastSeenDateMs: Date.now()
            }),
            awaitGlobalRegistration
        );
    } catch (e) {
        this._messageRouter.removeNextHop(participantId).catch(UtilInternal.emptyFunction);
        throw e;
    }

    log.info(
        `Provider registered: participantId: ${participantId}, domain: ${domain}, interfaceName: ${
            provider.interfaceName
        }`
    );
    return participantId;
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
CapabilitiesRegistrar.prototype.unregisterProvider = async function unregisterProvider(domain, provider) {
    this._checkIfReady();
    // retrieve participantId
    const participantId = this._participantIdStorage.getParticipantId(domain, provider);

    await this._discoveryStub.remove(participantId);

    // unregister routing address at routingTable
    await this._messageRouter.removeNextHop(participantId);

    // if provider has at least one attribute, remove it as publication
    // provider
    this._publicationManager.removePublicationProvider(participantId, provider);

    // unregister provider at RequestReplyManager
    this._requestReplyManager.removeRequestCaller(participantId);

    log.info(
        `Provider unregistered: participantId: ${participantId}, domain: ${domain}, interfaceName: ${
            provider.interfaceName
        }`
    );
};

/**
 * Shutdown the capabilities registrar
 *
 * @function
 * @name CapabilitiesRegistrar#shutdown
 */
CapabilitiesRegistrar.prototype.shutdown = function shutdown() {
    this._started = false;
};

CapabilitiesRegistrar.setDefaultExpiryIntervalMs = function(delay) {
    defaultExpiryIntervalMs = delay;
};

module.exports = CapabilitiesRegistrar;
