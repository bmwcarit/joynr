/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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
/*eslint promise/catch-or-return: "off"*/
const GlobalDiscoveryEntry = require("../../generated/joynr/types/GlobalDiscoveryEntry");
const CapabilityDiscovery = require("../capabilities/discovery/CapabilityDiscovery");
const CapabilitiesStore = require("../capabilities/CapabilitiesStore");
const ChannelAddress = require("../../generated/joynr/system/RoutingTypes/ChannelAddress");
const MqttMessagingStubFactory = require("../messaging/mqtt/MqttMessagingStubFactory");
const MqttMessagingSkeleton = require("../messaging/mqtt/MqttMessagingSkeleton");
const MqttAddress = require("../../generated/joynr/system/RoutingTypes/MqttAddress");
const SharedMqttClient = require("../messaging/mqtt/SharedMqttClient");
const MqttMulticastAddressCalculator = require("../messaging/mqtt/MqttMulticastAddressCalculator");
const MessagingSkeletonFactory = require("../messaging/MessagingSkeletonFactory");
const MessagingStubFactory = require("../messaging/MessagingStubFactory");
const InProcessSkeleton = require("../util/InProcessSkeleton");
const InProcessMessagingStubFactory = require("../messaging/inprocess/InProcessMessagingStubFactory");
const InProcessAddress = require("../messaging/inprocess/InProcessAddress");
const MessagingQos = require("../messaging/MessagingQos");
const DiscoveryQos = require("../proxy/DiscoveryQos");
const DiscoveryScope = require("../../generated/joynr/types/DiscoveryScope");
const UtilInternal = require("../util/UtilInternal");
const CapabilitiesUtil = require("../util/CapabilitiesUtil");
const loggingManager = require("../system/LoggingManager");
const nanoid = require("nanoid");
const defaultLibjoynrSettings = require("./settings/defaultLibjoynrSettings");
const defaultClusterControllerSettings = require("./settings/defaultClusterControllerSettings");
const Typing = require("../util/Typing");
const LongTimer = require("../util/LongTimer");
const JoynrStates = {
    SHUTDOWN: "shut down",
    STARTING: "starting",
    STARTED: "started",
    SHUTTINGDOWN: "shutting down"
};

const JoynrRuntime = require("./JoynrRuntime");

let clusterControllerSettings;

const log = loggingManager.getLogger("joynr.start.InProcessRuntime");

/**
 * The InProcessRuntime is the version of the libjoynr-js runtime that hosts its own
 * cluster controller
 *
 * @name InProcessRuntime
 * @constructor
 *
 * @param provisioning
 */
class InProcessRuntime extends JoynrRuntime {
    constructor() {
        super();

        this._freshnessIntervalId = null;
    }

    /**
     * Starts up the libjoynr instance
     *
     * @name InProcessRuntime#start
     * @function
     * @returns {Object} an A+ promise object, reporting when libjoynr startup is
     *          then({InProcessRuntime}, {Error})-ed
     * @throws {Error} if libjoynr is not in SHUTDOWN state
     */
    async start(provisioning) {
        super.start(provisioning);

        if (UtilInternal.checkNullUndefined(provisioning.bounceProxyUrl)) {
            throw new Error("bounce proxy URL not set in provisioning.bounceProxyUrl");
        }
        if (UtilInternal.checkNullUndefined(provisioning.bounceProxyBaseUrl)) {
            throw new Error("bounce proxy base URL not set in provisioning.bounceProxyBaseUrl");
        }
        if (UtilInternal.checkNullUndefined(provisioning.brokerUri)) {
            throw new Error("broker URI not set in provisioning.brokerUri");
        }

        const initialRoutingTable = {};

        await this._initializePersistency(provisioning);

        const channelId =
            provisioning.channelId ||
            (this._persistency && this._persistency.getItem("joynr.channels.channelId.1")) ||
            `chjs_${nanoid()}`;
        if (this._persistency) this._persistency.setItem("joynr.channels.channelId.1", channelId);

        let untypedCapabilities = provisioning.capabilities || [];
        clusterControllerSettings = defaultClusterControllerSettings({
            bounceProxyBaseUrl: provisioning.bounceProxyBaseUrl,
            brokerUri: provisioning.brokerUri
        });
        const defaultClusterControllerCapabilities = clusterControllerSettings.capabilities || [];

        untypedCapabilities = untypedCapabilities.concat(defaultClusterControllerCapabilities);
        // allow use of _typeName once
        this.typeRegistry.addType(new ChannelAddress()._typeName, ChannelAddress, false);
        this.typeRegistry.addType(new MqttAddress()._typeName, MqttAddress, false);

        const typedCapabilities = [];
        for (let i = 0; i < untypedCapabilities.length; i++) {
            const capability = new GlobalDiscoveryEntry(untypedCapabilities[i]);
            if (!capability.address) {
                throw new Error(`provisioned capability is missing address: ${JSON.stringify(capability)}`);
            }
            initialRoutingTable[capability.participantId] = Typing.augmentTypes(JSON.parse(capability.address));
            typedCapabilities.push(capability);
        }

        const globalClusterControllerAddress = new MqttAddress({
            brokerUri: provisioning.brokerUri,
            topic: channelId
        });
        const serializedGlobalClusterControllerAddress = JSON.stringify(globalClusterControllerAddress);

        const mqttClient = new SharedMqttClient({
            address: globalClusterControllerAddress,
            provisioning: provisioning.mqtt || {}
        });

        const messagingSkeletonFactory = new MessagingSkeletonFactory();

        const messagingStubFactories = {};
        messagingStubFactories[InProcessAddress._typeName] = new InProcessMessagingStubFactory();
        //messagingStubFactories[ChannelAddress._typeName] = channelMessagingStubFactory;
        messagingStubFactories[MqttAddress._typeName] = new MqttMessagingStubFactory({
            client: mqttClient,
            address: globalClusterControllerAddress
        });

        const messagingStubFactory = new MessagingStubFactory({
            messagingStubFactories
        });

        const messageRouterSettings = {
            initialRoutingTable,
            joynrInstanceId: channelId,
            typeRegistry: this.typeRegistry,
            messagingStubFactory,
            multicastAddressCalculator: new MqttMulticastAddressCalculator({
                globalAddress: globalClusterControllerAddress
            })
        };

        super._initializeComponents(provisioning, messageRouterSettings);

        const mqttMessagingSkeleton = new MqttMessagingSkeleton({
            address: globalClusterControllerAddress,
            client: mqttClient,
            messageRouter: this._messageRouter
        });

        this._messagingSkeletons[MqttAddress._typeName] = mqttMessagingSkeleton;
        messagingSkeletonFactory.setSkeletons(this._messagingSkeletons);

        this._messageRouter.setReplyToAddress(serializedGlobalClusterControllerAddress);

        const localCapabilitiesStore = new CapabilitiesStore(
            CapabilitiesUtil.toDiscoveryEntries(defaultLibjoynrSettings.capabilities || [])
        );
        const globalCapabilitiesCache = new CapabilitiesStore(typedCapabilities);

        const internalMessagingQos = new MessagingQos(provisioning.internalMessagingQos);

        const defaultProxyBuildSettings = {
            domain: "io.joynr",
            messagingQos: internalMessagingQos,
            discoveryQos: new DiscoveryQos({
                discoveryScope: DiscoveryScope.GLOBAL_ONLY,
                cacheMaxAgeMs: UtilInternal.getMaxLongValue()
            })
        };

        const capabilityDiscovery = new CapabilityDiscovery(
            localCapabilitiesStore,
            globalCapabilitiesCache,
            this._messageRouter,
            this.proxyBuilder,
            defaultProxyBuildSettings.domain
        );

        mqttClient.onConnected().then(() => {
            // TODO remove workaround when multiple backend support is implemented in JS
            const globalClusterControllerAddressWithGbid = new MqttAddress({
                brokerUri: "joynrtestgbid",
                topic: globalClusterControllerAddress.topic
            });
            capabilityDiscovery.globalAddressReady(globalClusterControllerAddressWithGbid);
        });

        this._discovery.setSkeleton(new InProcessSkeleton(capabilityDiscovery));

        const period = provisioning.capabilitiesFreshnessUpdateIntervalMs || 3600000; // default: 1 hour
        this._freshnessIntervalId = LongTimer.setInterval(() => {
            capabilityDiscovery.touch(channelId, period).catch(error => {
                log.error(`error sending freshness update: ${error}`);
            });
            return null;
        }, period);

        if (provisioning.logging) {
            loggingManager.configure(provisioning.logging);
        }

        this._joynrState = JoynrStates.STARTED;
        this._publicationManager.restore();
        log.debug("joynr initialized");
    }

    /**
     * Shuts down libjoynr
     *
     * @name InProcessRuntime#shutdown
     * @function
     * @throws {Error} if libjoynr is not in the STARTED state
     */
    shutdown(settings) {
        LongTimer.clearInterval(this._freshnessIntervalId);
        return super.shutdown(settings);
    }
}

module.exports = InProcessRuntime;
