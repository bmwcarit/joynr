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
const ProxyAttribute = require("./ProxyAttribute");
const ProxyOperation = require("./ProxyOperation");
const ProxyEvent = require("./ProxyEvent");
const uuid = require("uuid/v4");
const DiscoveryQos = require("./DiscoveryQos");
const MessagingQos = require("../messaging/MessagingQos");
const TypeRegistrySingleton = require("../../joynr/types/TypeRegistrySingleton");
const Version = require("../../generated/joynr/types/Version");
const Typing = require("../util/Typing");
const LoggingManager = require("../system/LoggingManager");

const proxyElementTypes = {
    ProxyAttribute,
    ProxyOperation,
    ProxyEvent
};
const typeRegistry = TypeRegistrySingleton.getInstance();

/**
 * @name ProxyBuilder
 * @constructor
 *
 * @param {Object}
 *            proxyDependencies injected for the children of the proxyBuilder and its children
 * @param {Arbitrator}
 *            proxyDependencies.arbitrator used by the proxyBuilder to find the correct provider
 * @param {RequestReplyManager}
 *            proxyDependencies.requestReplyManager passed on to proxyAttribute and
 *            proxyOperation
 * @param {SubscriptionManager}
 *            proxyDependencies.subscriptionManager passed on to proxyAttribute and
 *            proxyOperation
 * @param {PublicationManager}
 *            proxyDependencies.publicationManager passed on to proxyAttribute and
 *            proxyOperation
 * @param {Object}
 *            dependencies injected for the proxyBuilder
 * @param {MessageRouter}
 *            dependencies.messageRouter the message router
 * @param {LoggingManager}
 *            dependencies.loggingManager provides the logging context
 * @param {Address}
 *            dependencies.libjoynrMessagingAddress address to this libjoynr's message receiver
 * @param {TypeRegistry}
 *            dependencies.typeRegistry the typeRegistry being able to augment raw objects with
 *            type information
 */
function ProxyBuilder(proxyDependencies, dependencies) {
    const arbitrator = proxyDependencies.arbitrator;
    const log = LoggingManager.getLogger("joynr.proxy.ProxyBuilder");
    const typeRegisteredTimeout_ms = 3000; //3 secs

    /**
     * A function that constructs, arbitrates a object and provides the result and error using
     * an A+ promise
     *
     * @name ProxyBuilder#build
     * @function
     *
     * @param {Function}
     *            ProxyConstructor - the constructor function of the generated Proxy that
     *            creates a new proxy instance
     * @param {Object}
     *            settings - the settings object that is passed to the constructor when building
     *            the object
     * @param {String}
     *            settings.domain - the domain on which the provider should be looked for
     * @param {DiscoveryQos}
     *            settings.discoveryQos - the settings object determining arbitration
     *            parameters
     * @param {MessagingQos}
     *            settings.messagingQos - the settings object determining messaging quality of
     *            service parameters
     * @param {Boolean}
     *            settings.freeze - define if the returned proxy object should be frozen
     * @param {Object}
     *            settings.loggingContext - optional logging context will be appended to logging
     *            messages created in the name of this proxy
     * @returns {Object} an A Promise object, that will provide the proxy object upon completed
     *            arbitration, callback signatures: then({*Proxy} proxy), {Error} error)
     * @throws {Error}
     *            if arbitrator was not provided
     */
    this.build = function build(ProxyConstructor, settings) {
        // augment Qos objects if they're missing
        settings.discoveryQos = new DiscoveryQos(settings.discoveryQos);
        settings.messagingQos = new MessagingQos(settings.messagingQos);

        // check if objects are there and of correct type
        Typing.checkProperty(settings, "Object", "settings");
        Typing.checkProperty(settings.domain, "String", "settings.domain");
        Typing.checkProperty(settings.discoveryQos, DiscoveryQos, "settings.discoveryQos");
        Typing.checkProperty(settings.messagingQos, MessagingQos, "settings.messagingQos");

        if (!arbitrator) {
            throw new Error("value 'arbitrator' is undefined");
        }

        settings.dependencies = proxyDependencies;
        settings.proxyElementTypes = proxyElementTypes;
        let proxy = new ProxyConstructor(settings);
        proxy.domain = settings.domain;
        proxy.proxyParticipantId = uuid();
        proxy.messagingQos = settings.messagingQos;

        const datatypePromises = ProxyConstructor.getUsedDatatypes().map(datatype => {
            return typeRegistry.getTypeRegisteredPromise(datatype, typeRegisteredTimeout_ms);
        });

        function dataTypePromisesOnSuccess() {
            const proxyVersion = new Version({
                majorVersion: proxy.constructor.MAJOR_VERSION,
                minorVersion: proxy.constructor.MINOR_VERSION
            });
            return arbitrator.startArbitration({
                domains: [proxy.domain],
                interfaceName: proxy.interfaceName,
                discoveryQos: settings.discoveryQos,
                staticArbitration: settings.staticArbitration,
                proxyVersion
            });
        }

        function startArbitrationOnSuccess(arbitratedCaps) {
            if (settings.loggingContext !== undefined) {
                log.warn("loggingContext is currently not supported");
            }
            let isGloballyVisible = false;
            if (arbitratedCaps && arbitratedCaps.length > 0) {
                proxy.providerDiscoveryEntry = arbitratedCaps[0];
                if (!arbitratedCaps[0].isLocal) {
                    isGloballyVisible = true;
                }
            }

            dependencies.messageRouter
                .addNextHop(proxy.proxyParticipantId, dependencies.libjoynrMessagingAddress, isGloballyVisible)
                .catch(error => {
                    log.debug(
                        `Exception occured while registering the address for interface ${proxy.interfaceName}, domain ${
                            proxy.domain
                        }, proxyParticipantId ${proxy.proxyParticipantId} to message router. Error: ${error.stack}`
                    );
                });
            dependencies.messageRouter.setToKnown(proxy.providerDiscoveryEntry.participantId);

            const freeze = settings.freeze === undefined || settings.freeze;
            if (freeze) {
                // make proxy object immutable and return asynchronously
                proxy = Object.freeze(proxy);
            }

            log.info(
                `Proxy created, proxy participantId: ${
                    proxy.proxyParticipantId
                }, provider discoveryEntry: ${JSON.stringify(proxy.providerDiscoveryEntry)}`
            );

            return proxy;
        }

        return Promise.all(datatypePromises)
            .then(dataTypePromisesOnSuccess)
            .then(startArbitrationOnSuccess);
    };
}

module.exports = ProxyBuilder;
