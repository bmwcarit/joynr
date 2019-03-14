/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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
require("../../node-unit-test-helper");

const provisioning_root = require("../../../resources/joynr/provisioning/provisioning_root"); // logger and mqtt

const proxyquire = require("proxyquire").noCallThru();

const mocks = {};
const constructors = {};
const spys = {};
[
    ["DiscoveryQos"],
    ["CapabilitiesRegistrar", ["shutdown"]],
    ["MessageRouter", ["setRoutingProxy", "addNextHop", "shutdown"]],
    ["MessageQueue"],
    [
        "Dispatcher",
        [
            "registerRequestReplyManager",
            "registerSubscriptionManager",
            "registerPublicationManager",
            "registerMessageRouter",
            "shutdown"
        ]
    ],
    ["ParticipantIdStorage"],
    ["PublicationManager", ["restore", "shutdown"]],
    ["LocalStorageNode", ["shutdown", "getItem"]],
    ["MessagingQos"],
    ["WebSocketMessagingSkeleton", ["registerListener", "shutdown"]],
    ["SharedWebSocket", ["enableShutdownMode"]],
    ["WebSocketMessagingStubFactory"],
    ["JoynrMessage"],
    ["LoggingManager", ["configure", "getLogger"]],
    ["SubscriptionManager", ["shutdown", "terminateSubscriptions"]],
    ["ProxyBuilder", ["build"]]
].forEach(([name, keys = []]) => {
    mocks[name] = keys.length > 0 ? jasmine.createSpyObj(name, keys) : {};
    spys[name] = jasmine.createSpy();
    constructors[name] = function(...args) {
        spys[name](...args);
        return mocks[name];
    };
});

mocks.MessageRouter.setRoutingProxy = jasmine.createSpy().and.returnValue(Promise.resolve());
mocks.ProxyBuilder.build.and.returnValue(Promise.resolve());

mocks.SubscriptionManager.terminateSubscriptions.and.returnValue(Promise.resolve());

constructors.CapabilitiesRegistrar.setDefaultExpiryIntervalMs = jasmine.createSpy();
constructors.DiscoveryQos.setDefaultSettings = jasmine.createSpy();
constructors.JoynrMessage.setSigningCallback = jasmine.createSpy();

mocks.LocalStorageNode.init = jasmine.createSpy().and.returnValue(Promise.resolve());
mocks.LoggingManager.getLogger.and.returnValue(
    jasmine.createSpyObj("logger", ["debug", "info", "error", "warn", "verbose"])
);

const config = {
    "../../../../main/js/joynr/proxy/DiscoveryQos": constructors.DiscoveryQos,
    "../../../../main/js/joynr/capabilities/CapabilitiesRegistrar": constructors.CapabilitiesRegistrar,
    "../../../../main/js/joynr/messaging/routing/MessageRouter": constructors.MessageRouter,
    "../../../../main/js/joynr/messaging/routing/MessageQueue": constructors.MessageQueue,
    "../../../../main/js/joynr/dispatching/Dispatcher": constructors.Dispatcher,
    "../../../../main/js/joynr/capabilities/ParticipantIdStorage": constructors.ParticipantIdStorage,
    "../../../../main/js/joynr/dispatching/subscription/PublicationManager": constructors.PublicationManager,
    "../../../../main/js/global/LocalStorageNode": constructors.LocalStorageNode,
    "../../../../main/js/joynr/messaging/MessagingQos": constructors.MessagingQos,
    "../../../../main/js/joynr/messaging/websocket/WebSocketMessagingSkeleton": constructors.WebSocketMessagingSkeleton,
    "../../../../main/js/joynr/messaging/websocket/SharedWebSocket": constructors.SharedWebSocket,
    "../../../../main/js/joynr/messaging/websocket/WebSocketMessagingStubFactory":
        constructors.WebSocketMessagingStubFactory,
    "../../../../main/js/joynr/messaging/JoynrMessage": constructors.JoynrMessage,
    "../../../../main/js/joynr/dispatching/subscription/SubscriptionManager": constructors.SubscriptionManager,
    "../../../../main/js/joynr/system/LoggingManager": mocks.LoggingManager,
    "../../../../main/js/joynr/proxy/ProxyBuilder": constructors.ProxyBuilder
};

const JoynrRuntime = proxyquire("joynr/joynr/start/JoynrRuntime", config);
const WebSocketLibjoynrRuntime = proxyquire(
    "joynr/joynr/start/WebSocketLibjoynrRuntime",
    Object.assign({}, config, { "../../../../main/js/joynr/start/JoynrRuntime": JoynrRuntime })
);

describe("libjoynr-js.joynr.start.WebSocketLibjoynrRuntime", () => {
    let runtime;
    let provisioning;

    beforeEach(() => {
        provisioning = Object.assign({}, provisioning_root, {
            ccAddress: {
                protocol: "ws",
                host: "localhost",
                port: 4242,
                path: ""
            }
        });

        // unfortunately jasmine doesn't reset spies between specs per default ...
        Object.keys(spys).forEach(spy => {
            spys[spy].calls.reset();
        });
        mocks.SubscriptionManager.terminateSubscriptions.calls.reset();
    });

    it("won't override settings unnecessarily", async () => {
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        await runtime.shutdown();
        expect(constructors.DiscoveryQos.setDefaultSettings).not.toHaveBeenCalled();
        expect(constructors.CapabilitiesRegistrar.setDefaultExpiryIntervalMs).not.toHaveBeenCalled();
    });

    it("will set the default discoveryQos settings correctly", async () => {
        const discoveryRetryDelayMs = 100;
        const discoveryTimeoutMs = 200;
        const discoveryExpiryIntervalMs = 100;
        provisioning.discoveryQos = {
            discoveryRetryDelayMs,
            discoveryTimeoutMs,
            discoveryExpiryIntervalMs
        };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        await runtime.shutdown();

        expect(constructors.DiscoveryQos.setDefaultSettings).toHaveBeenCalledWith({
            discoveryRetryDelayMs,
            discoveryTimeoutMs
        });
        expect(constructors.CapabilitiesRegistrar.setDefaultExpiryIntervalMs).toHaveBeenCalledWith(
            discoveryExpiryIntervalMs
        );
    });

    it("will initialize SharedWebSocket correctly", async () => {
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);

        expect(spys.SharedWebSocket).toHaveBeenCalledWith({
            remoteAddress: jasmine.objectContaining({
                _typeName: "joynr.system.RoutingTypes.WebSocketAddress",
                protocol: provisioning.ccAddress.protocol,
                host: provisioning.ccAddress.host,
                port: provisioning.ccAddress.port,
                path: provisioning.ccAddress.path
            }),
            localAddress: jasmine.objectContaining({
                _typeName: "joynr.system.RoutingTypes.WebSocketClientAddress"
            }),
            provisioning: jasmine.any(Object),
            keychain: undefined
        });
    });

    it("will use the default persistency settings", async () => {
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(spys.MessageRouter.calls.count()).toEqual(1);
        expect(spys.MessageRouter.calls.argsFor(0)[0].persistency).toBeUndefined();
        expect(spys.ParticipantIdStorage.calls.count()).toEqual(1);
        expect(spys.ParticipantIdStorage.calls.argsFor(0)[0]).toEqual(mocks.LocalStorageNode);
        expect(spys.PublicationManager.calls.count()).toEqual(1);
        expect(spys.PublicationManager.calls.argsFor(0)[1]).toEqual(mocks.LocalStorageNode);
    });

    it("enables MessageRouter Persistency if configured", async () => {
        provisioning.persistency = { routingTable: true };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(spys.MessageRouter.calls.count()).toEqual(1);
        expect(spys.MessageRouter.calls.argsFor(0)[0].persistency).toEqual(mocks.LocalStorageNode);
        expect(spys.MessageRouter).toHaveBeenCalledWith(
            jasmine.objectContaining({ persistency: mocks.LocalStorageNode })
        );
    });

    it("enables ParticipantIdStorage persistency if configured", async () => {
        provisioning.persistency = { capabilities: true };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(spys.ParticipantIdStorage.calls.count()).toEqual(1);
        expect(spys.ParticipantIdStorage.calls.argsFor(0)[0]).toEqual(mocks.LocalStorageNode);
    });

    it("disables PublicationManager persistency if configured", async () => {
        provisioning.persistency = { publications: false };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(spys.PublicationManager.calls.count()).toEqual(1);
        expect(spys.PublicationManager.calls.argsFor(0)[1]).toBeUndefined();
    });

    it("will call MessageQueue with the settings from the provisioning", async () => {
        const maxQueueSizeInKBytes = 100;
        provisioning.messaging = { maxQueueSizeInKBytes };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(spys.MessageQueue.calls.count()).toEqual(1);
        expect(spys.MessageQueue).toHaveBeenCalledWith({
            maxQueueSizeInKBytes
        });
    });

    it("will call Dispatcher with the settings from the provisioning", async () => {
        const ttlUpLiftMs = 1000;
        provisioning.messaging = { TTL_UPLIFT: ttlUpLiftMs };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(spys.Dispatcher.calls.count()).toEqual(1);
        expect(spys.Dispatcher.calls.argsFor(0)[2]).toEqual(ttlUpLiftMs);
    });

    it("will call MessagingQos with the settings from the provisioning", async () => {
        const ttl = 1000;
        provisioning.internalMessagingQos = { ttl };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(spys.MessagingQos).toHaveBeenCalledWith({ ttl });
    });

    it("will set the signingCallback to the joynrMessage.prototype", async () => {
        provisioning.keychain = {
            tlsCert: "tlsCert",
            tlsKey: "tlsKey",
            tlsCa: "tlsCa",
            ownerId: "ownerID"
        };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        await runtime.shutdown();
        expect(constructors.JoynrMessage.setSigningCallback).toHaveBeenCalled();
    });

    it("calls SharedWebSocket.enableShutdownMode in terminateAllSubscriptions", async () => {
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        await runtime.terminateAllSubscriptions();
        expect(mocks.SharedWebSocket.enableShutdownMode).toHaveBeenCalled();
        await runtime.shutdown();
    });

    it("calls SubscriptionManager.terminateSubscriptions in terminateAllSubscriptions", async () => {
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        await runtime.terminateAllSubscriptions();
        expect(mocks.SubscriptionManager.terminateSubscriptions).toHaveBeenCalledWith(0);
        await runtime.shutdown();
    });

    it("terminates Subscriptions upon shutdown with default timeout", done => {
        runtime = new WebSocketLibjoynrRuntime();
        runtime
            .start(provisioning)
            .then(runtime.shutdown)
            .then(() => {
                expect(mocks.SubscriptionManager.terminateSubscriptions).toHaveBeenCalledWith(1000);
                done();
            })
            .catch(fail);
    });

    it("won't terminate Subscriptions upon shutdown when specified by provisioning", done => {
        runtime = new WebSocketLibjoynrRuntime();

        provisioning.shutdownSettings = { clearSubscriptionsEnabled: false };
        runtime
            .start(provisioning)
            .then(() => {
                return runtime.shutdown();
            })
            .then(() => {
                expect(mocks.SubscriptionManager.terminateSubscriptions).not.toHaveBeenCalled();
                done();
            })
            .catch(fail);
    });

    it("won't terminate Subscriptions when explicitly called with shutdown", done => {
        runtime = new WebSocketLibjoynrRuntime();

        provisioning.shutdownSettings = { clearSubscriptionsEnabled: false };
        runtime
            .start(provisioning)
            .then(() => {
                runtime.shutdown({ clearSubscriptionsEnabled: false });
            })
            .then(() => {
                expect(mocks.SubscriptionManager.terminateSubscriptions).not.toHaveBeenCalled();
                done();
            })
            .catch(fail);
    });

    it("calls enableShutdownMode of SharedWebsocket before when shut down", async () => {
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        await runtime.shutdown();
        expect(mocks.SharedWebSocket.enableShutdownMode).toHaveBeenCalled();
    });
});
