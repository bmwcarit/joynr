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
import * as WebSocketProtocol from "../../../../main/js/generated/joynr/system/RoutingTypes/WebSocketProtocol";
import ProvisioningRoot from "../../../resources/joynr/provisioning/provisioning_root"; // logger and mqtt

// @ts-ignore
const onFatalRuntimeError = (error: JoynrRuntimeException) => {
    return;
};

const mocks: Record<string, any> = {};
const constructors: Record<string, any> = {};
const spies: Record<string, any> = {};
[
    ["DiscoveryQos"],
    ["CapabilitiesRegistrar", ["shutdown"]],
    ["MessageRouter", ["setRoutingProxy", "addNextHop", "shutdown", "configureReplyToAddressFromRoutingProxy"]],
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
].forEach(([name, keys = []]: any) => {
    mocks[name] = {};
    keys.forEach((key: string) => {
        mocks[name][key] = jest.fn();
    });
    spies[name] = jest.fn();
    constructors[name] = function(...args: any[]) {
        spies[name](...args);
        return mocks[name];
    };
});

mocks.MessageRouter.setRoutingProxy = jest.fn().mockReturnValue(Promise.resolve());
mocks.MessageRouter.configureReplyToAddressFromRoutingProxy = jest.fn().mockReturnValue(Promise.resolve());
mocks.ProxyBuilder.build.mockReturnValue(Promise.resolve());

mocks.SubscriptionManager.terminateSubscriptions.mockReturnValue(Promise.resolve());

constructors.CapabilitiesRegistrar.setDefaultExpiryIntervalMs = jest.fn();
constructors.DiscoveryQos.setDefaultSettings = jest.fn();
constructors.JoynrMessage.setSigningCallback = jest.fn();

mocks.LocalStorageNode.init = jest.fn().mockReturnValue(Promise.resolve());
mocks.LoggingManager.getLogger.mockReturnValue({
    debug: jest.fn(),
    info: jest.fn(),
    error: jest.fn(),
    warn: jest.fn(),
    verbose: jest.fn()
});

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

for (const [key, value] of Object.entries(config)) {
    jest.doMock(key, () => value);
}
import WebSocketLibjoynrRuntime from "../../../../main/js/joynr/start/WebSocketLibjoynrRuntime";
import JoynrRuntimeException from "../../../../main/js/joynr/exceptions/JoynrRuntimeException";

describe("libjoynr-js.joynr.start.WebSocketLibjoynrRuntime", () => {
    let runtime: any;
    let provisioning: any;

    beforeEach(() => {
        provisioning = Object.assign({}, ProvisioningRoot, {
            ccAddress: {
                protocol: "ws",
                host: "localhost",
                port: 4242,
                path: ""
            }
        });

        // unfortunately jasmine doesn't reset spies between specs per default ...
        Object.keys(spies).forEach((spy: any) => {
            spies[spy].mockClear();
        });
        mocks.SubscriptionManager.terminateSubscriptions.mockClear();
    });

    it("won't override settings unnecessarily", async () => {
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
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
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
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
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);

        expect(spies.SharedWebSocket).toHaveBeenCalledWith({
            remoteAddress: expect.objectContaining({
                _typeName: "joynr.system.RoutingTypes.WebSocketAddress",
                protocol: WebSocketProtocol.WS,
                host: provisioning.ccAddress.host,
                port: provisioning.ccAddress.port,
                path: provisioning.ccAddress.path
            }),
            localAddress: expect.objectContaining({
                _typeName: "joynr.system.RoutingTypes.WebSocketClientAddress"
            }),
            provisioning: expect.any(Object),
            keychain: undefined
        });
        await runtime.shutdown();
    });

    it("will use the default persistency settings", async () => {
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);
        expect(spies.MessageRouter.mock.calls.length).toEqual(1);
        expect(spies.MessageRouter.mock.calls[0][0].persistency).toBeUndefined();
        expect(spies.ParticipantIdStorage.mock.calls.length).toEqual(1);
        expect(spies.ParticipantIdStorage.mock.calls[0][0]).toEqual(mocks.LocalStorageNode);
        expect(spies.PublicationManager.mock.calls.length).toEqual(1);
        expect(spies.PublicationManager.mock.calls[0][1]).toEqual(mocks.LocalStorageNode);
        await runtime.shutdown();
    });

    it("enables MessageRouter Persistency if configured", async () => {
        provisioning.persistency = { routingTable: true };
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);
        expect(spies.MessageRouter.mock.calls.length).toEqual(1);
        expect(spies.MessageRouter.mock.calls[0][0].persistency).toEqual(mocks.LocalStorageNode);
        expect(spies.MessageRouter).toHaveBeenCalledWith(
            expect.objectContaining({ persistency: mocks.LocalStorageNode })
        );
        await runtime.shutdown();
    });

    it("enables ParticipantIdStorage persistency if configured", async () => {
        provisioning.persistency = { capabilities: true };
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);
        expect(spies.ParticipantIdStorage.mock.calls.length).toEqual(1);
        expect(spies.ParticipantIdStorage.mock.calls[0][0]).toEqual(mocks.LocalStorageNode);
        await runtime.shutdown();
    });

    it("disables PublicationManager persistency if configured", async () => {
        provisioning.persistency = { publications: false };
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);
        expect(spies.PublicationManager.mock.calls.length).toEqual(1);
        expect(spies.PublicationManager.mock.calls[0][1]).toBeUndefined();
        await runtime.shutdown();
    });

    it("will call MessageQueue with the settings from the provisioning", async () => {
        const maxQueueSizeInKBytes = 100;
        provisioning.messaging = { maxQueueSizeInKBytes };
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);
        expect(spies.MessageQueue.mock.calls.length).toEqual(1);
        expect(spies.MessageQueue).toHaveBeenCalledWith({
            maxQueueSizeInKBytes
        });
        await runtime.shutdown();
    });

    it("will call Dispatcher with the settings from the provisioning", async () => {
        const ttlUpLiftMs = 1000;
        provisioning.messaging = { TTL_UPLIFT: ttlUpLiftMs };
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);
        expect(spies.Dispatcher.mock.calls.length).toEqual(1);
        expect(spies.Dispatcher.mock.calls[0][2]).toEqual(ttlUpLiftMs);
        await runtime.shutdown();
    });

    it("will call MessagingQos with the settings from the provisioning", async () => {
        const ttl = 1000;
        provisioning.internalMessagingQos = { ttl };
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);
        expect(spies.MessagingQos).toHaveBeenCalledWith({ ttl });
        await runtime.shutdown();
    });

    it("will set the signingCallback to the joynrMessage.prototype", async () => {
        provisioning.keychain = {
            tlsCert: "tlsCert",
            tlsKey: "tlsKey",
            tlsCa: "tlsCa",
            ownerId: "ownerID"
        };
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);
        await runtime.shutdown();
        expect(constructors.JoynrMessage.setSigningCallback).toHaveBeenCalled();
    });

    it("calls SharedWebSocket.enableShutdownMode in terminateAllSubscriptions", async () => {
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);
        await runtime.terminateAllSubscriptions();
        expect(mocks.SharedWebSocket.enableShutdownMode).toHaveBeenCalled();
        await runtime.shutdown();
    });

    it("calls SubscriptionManager.terminateSubscriptions in terminateAllSubscriptions", async () => {
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);
        await runtime.terminateAllSubscriptions();
        expect(mocks.SubscriptionManager.terminateSubscriptions).toHaveBeenCalledWith(0);
        await runtime.shutdown();
    });

    it("terminates Subscriptions upon shutdown with default timeout", async () => {
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);

        await runtime.start(provisioning);
        await runtime.shutdown();

        expect(mocks.SubscriptionManager.terminateSubscriptions).toHaveBeenCalledWith(1000);
    });

    it("won't terminate Subscriptions upon shutdown when specified by provisioning", async () => {
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);

        provisioning.shutdownSettings = { clearSubscriptionsEnabled: false };

        await runtime.start(provisioning);

        await runtime.shutdown();
        expect(mocks.SubscriptionManager.terminateSubscriptions).not.toHaveBeenCalled();
    });

    it("won't terminate Subscriptions when explicitly called with shutdown", async () => {
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);

        await runtime.start(provisioning);

        runtime.shutdown({ clearSubscriptionsEnabled: false });
        expect(mocks.SubscriptionManager.terminateSubscriptions).not.toHaveBeenCalled();
    });

    it("calls enableShutdownMode of SharedWebsocket before when shut down", async () => {
        runtime = new WebSocketLibjoynrRuntime(onFatalRuntimeError);
        await runtime.start(provisioning);
        await runtime.shutdown();
        expect(mocks.SharedWebSocket.enableShutdownMode).toHaveBeenCalled();
    });
});
