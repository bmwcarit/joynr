/*
 * #%L
 * %%
 * Copyright (C) 2020 BMW Car IT GmbH
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

import ProvisioningRoot from "../../../resources/joynr/provisioning/provisioning_root";

const onFatalRuntimeErrorSpy = jest.fn();

const onFatalRuntimeErrorCallBack = (error: JoynrRuntimeException) => {
    onFatalRuntimeErrorSpy(error);
};

const mocks: Record<string, any> = {};
const constructors: Record<string, any> = {};
const spies: Record<string, any> = {};
[
    ["DiscoveryQos"],
    ["CapabilitiesRegistrar", ["shutdown"]],
    [
        "MessageRouter",
        [
            "setRoutingProxy",
            "addNextHop",
            "shutdown",
            "configureReplyToAddressFromRoutingProxy",
            "getReplyToAddressFromRoutingProxy"
        ]
    ],
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
    ["LoggingManager", ["configure", "getLogger", "registerForLogLevelChanged"]],
    ["SubscriptionManager", ["shutdown", "terminateSubscriptions"]],
    ["ProxyBuilder", ["build"]],
    ["MessageReplyToAddressCalculator", ["setReplyTo", "setReplyToAddress"]],
    ["UdsClient", ["enableShutdownMode", "shutdown"]],
    ["UdsAddress"],
    ["UdsClientAddress"]
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
    "../../../../main/js/joynr/dispatching/subscription/SubscriptionManager": constructors.SubscriptionManager,
    "../../../../main/js/joynr/system/LoggingManager": mocks.LoggingManager,
    "../../../../main/js/joynr/messaging/JoynrMessage": constructors.JoynrMessage,
    "../../../../main/js/joynr/proxy/ProxyBuilder": constructors.ProxyBuilder,
    "../../../../main/js/joynr/messaging/MessageReplyToAddressCalculator": constructors.MessageReplyToAddressCalculator,
    "../../../../main/js/joynr/messaging/uds/UdsClient": constructors.UdsClient,
    "../../../../main/js/generated/joynr/system/RoutingTypes/UdsAddress": constructors.UdsAddress,
    "../../../../main/js/generated/joynr/system/RoutingTypes/UdsClientAddress": constructors.UdsClientAddress
};

for (const [key, value] of Object.entries(config)) {
    jest.doMock(key, () => value);
}

import UdsLibJoynrRuntime from "../../../../main/js/joynr/start/UdsLibJoynrRuntime";
import JoynrRuntimeException from "../../../../main/js/joynr/exceptions/JoynrRuntimeException";

describe("libjoynr-js.joynr.start.UdsLibJoynrRuntimeTest", () => {
    let runtime: any;
    let provisioning: any;

    beforeEach(() => {
        constructors.CapabilitiesRegistrar.setDefaultExpiryIntervalMs = jest.fn();
        constructors.DiscoveryQos.setDefaultSettings = jest.fn();
        mocks.MessageRouter.setRoutingProxy = jest.fn().mockReturnValue(Promise.resolve());
        mocks.MessageRouter.getReplyToAddressFromRoutingProxy = jest.fn().mockReturnValue(Promise.resolve());
        mocks.ProxyBuilder.build.mockReturnValue(Promise.resolve());

        mocks.SubscriptionManager.terminateSubscriptions.mockReturnValue(Promise.resolve());

        mocks.LocalStorageNode.init = jest.fn().mockReturnValue(Promise.resolve());

        provisioning = Object.assign({}, ProvisioningRoot, {
            uds: {
                socketPath: "/tmp/libjoynr-js.joynr.start.UdsLibJoynrRuntimeTest.sock",
                clientId: "test_udsClientId",
                connectSleepTimeMs: 404
            }
        });

        Object.keys(spies).forEach((spy: any) => {
            spies[spy].mockClear();
        });
        mocks.SubscriptionManager.terminateSubscriptions.mockClear();
    });

    it("won't override settings unnecessarily", async () => {
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
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
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
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

    it("will initialize UdsClient correctly when provisioning is provided", async () => {
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);

        const expectedSocketPath = provisioning.uds.socketPath;
        const expectedClient = provisioning.uds.clientId;
        const expectedConnectSleepTimeMs = provisioning.uds.connectSleepTimeMs;

        expect(spies.UdsClient).toHaveBeenCalledWith({
            socketPath: expectedSocketPath,
            clientId: expectedClient,
            connectSleepTimeMs: expectedConnectSleepTimeMs,
            onMessageCallback: expect.anything(),
            onFatalRuntimeError: onFatalRuntimeErrorCallBack
        });
        await runtime.shutdown();
    });

    it("will initialize UdsClient with default values when provisioning undefined", async () => {
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        provisioning.uds = undefined;
        await runtime.start(provisioning);

        const expectedSocketPath = "/var/run/joynr/cluster-controller.sock";
        const expectedConnectSleepTimeMs = 500;

        expect(spies.UdsClient).toHaveBeenCalledWith({
            socketPath: expectedSocketPath,
            clientId: expect.anything(),
            connectSleepTimeMs: expectedConnectSleepTimeMs,
            onMessageCallback: expect.anything(),
            onFatalRuntimeError: onFatalRuntimeErrorCallBack
        });
        await runtime.shutdown();
    });

    it("will initialize UdsAddress (ccAddress) with correct socket path", async () => {
        const expectedSocketPath = provisioning.uds.socketPath;

        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);

        expect(spies.UdsAddress).toHaveBeenCalledWith({
            path: expectedSocketPath
        });
        await runtime.shutdown();
    });

    it("will initialize UdsClientAddress with correct client id", async () => {
        const expectedUdsClientId = provisioning.uds.clientId;

        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);

        expect(spies.UdsClientAddress).toHaveBeenCalledWith({
            id: expectedUdsClientId
        });
        await runtime.shutdown();
    });

    it("will set routing proxy after building of RoutingProxy", async () => {
        mocks.MessageRouter.setRoutingProxy.mockReturnValue(Promise.reject(new Error("error from setRoutingProxy")));

        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        try {
            await runtime.start(provisioning);
        } catch (e) {
            expect(e.message).toEqual(expect.stringContaining("error from setRoutingProxy"));
            expect(e.message).toEqual(expect.stringContaining("Failed to create routing proxy"));
        }

        expect(mocks.MessageRouter.setRoutingProxy).toHaveBeenCalled();
        expect(mocks.MessageRouter.getReplyToAddressFromRoutingProxy).not.toHaveBeenCalled();
        expect(mocks.MessageReplyToAddressCalculator.setReplyToAddress).not.toHaveBeenCalled();
    });

    it("will get replyToAddress after setting routing proxy in message router", async () => {
        mocks.MessageRouter.getReplyToAddressFromRoutingProxy.mockReturnValue(
            Promise.reject(new Error("error from getReplyToAddressFromRoutingProxy"))
        );
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        try {
            await runtime.start(provisioning);
        } catch (e) {
            expect(e.message).toEqual(expect.stringContaining("error from getReplyToAddressFromRoutingProxy"));
            expect(e.message).toEqual(expect.stringContaining("Failed to initialize replyToAddress"));
        }

        expect(mocks.MessageRouter.setRoutingProxy).toHaveBeenCalled();
        expect(mocks.MessageRouter.getReplyToAddressFromRoutingProxy).toHaveBeenCalled();
        expect(mocks.MessageReplyToAddressCalculator.setReplyToAddress).not.toHaveBeenCalled();
    });

    it("will set replyToAddress in messageReplyToAddressCalculator", async () => {
        mocks.MessageRouter.getReplyToAddressFromRoutingProxy = jest
            .fn()
            .mockReturnValue(Promise.resolve("testAddress"));
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);

        expect(mocks.MessageReplyToAddressCalculator.setReplyToAddress).toHaveBeenCalledWith("testAddress");
        await runtime.shutdown();
    });

    it("will use the default persistency settings", async () => {
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);
        expect(spies.MessageRouter.mock.calls.length).toEqual(1);
        expect(spies.MessageRouter.mock.calls[0][0].persistency).toBeUndefined();
        expect(spies.ParticipantIdStorage.mock.calls.length).toEqual(1);
        expect(spies.ParticipantIdStorage.mock.calls[0][0]).toEqual(mocks.LocalStorageNode);
        expect(spies.PublicationManager.mock.calls.length).toEqual(1);
        expect(spies.UdsClient.mock.calls.length).toEqual(1);
        expect(spies.PublicationManager.mock.calls[0][1]).toEqual(mocks.LocalStorageNode);
        await runtime.shutdown();
    });

    it("enables MessageRouter Persistency if configured", async () => {
        provisioning.persistency = { routingTable: true };
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
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
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);
        expect(spies.ParticipantIdStorage.mock.calls.length).toEqual(1);
        expect(spies.ParticipantIdStorage.mock.calls[0][0]).toEqual(mocks.LocalStorageNode);
        await runtime.shutdown();
    });

    it("disables PublicationManager persistency if configured", async () => {
        provisioning.persistency = { publications: false };
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);
        expect(spies.PublicationManager.mock.calls.length).toEqual(1);
        expect(spies.PublicationManager.mock.calls[0][1]).toBeUndefined();
        await runtime.shutdown();
    });

    it("will call MessageQueue with the settings from the provisioning", async () => {
        const maxQueueSizeInKBytes = 100;
        provisioning.messaging = { maxQueueSizeInKBytes };
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
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
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);
        expect(spies.Dispatcher.mock.calls.length).toEqual(1);
        expect(spies.Dispatcher.mock.calls[0][2]).toEqual(ttlUpLiftMs);
        await runtime.shutdown();
    });

    it("will call MessagingQos with the settings from the provisioning", async () => {
        const ttl = 1000;
        provisioning.internalMessagingQos = { ttl };
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);
        expect(spies.MessagingQos).toHaveBeenCalledWith({ ttl });
        await runtime.shutdown();
    });

    it("calls UdsClient.enableShutdownMode in terminateAllSubscriptions", async () => {
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);
        await runtime.terminateAllSubscriptions();
        expect(mocks.UdsClient.enableShutdownMode).toHaveBeenCalled();
        await runtime.shutdown();
    });

    it("calls SubscriptionManager.terminateSubscriptions in terminateAllSubscriptions", async () => {
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);
        await runtime.terminateAllSubscriptions();
        expect(mocks.SubscriptionManager.terminateSubscriptions).toHaveBeenCalledWith(0);
        await runtime.shutdown();
    });

    it("terminates Subscriptions upon shutdown with default timeout", async () => {
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);

        await runtime.start(provisioning);
        await runtime.shutdown();

        expect(mocks.SubscriptionManager.terminateSubscriptions).toHaveBeenCalledWith(1000);
    });

    it("won't terminate Subscriptions upon shutdown when specified by provisioning", async () => {
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);

        provisioning.shutdownSettings = { clearSubscriptionsEnabled: false };

        await runtime.start(provisioning);

        await runtime.shutdown();
        expect(mocks.SubscriptionManager.terminateSubscriptions).not.toHaveBeenCalled();
    });

    it("won't terminate Subscriptions when explicitly called with shutdown", async () => {
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);

        await runtime.start(provisioning);

        await runtime.shutdown({ clearSubscriptionsEnabled: false });
        expect(mocks.SubscriptionManager.terminateSubscriptions).not.toHaveBeenCalled();
    });

    it("will call enableShutdownMode and shutdown of UdsClient when shut down", async () => {
        runtime = new UdsLibJoynrRuntime(onFatalRuntimeErrorCallBack);
        await runtime.start(provisioning);
        await runtime.shutdown();

        expect(mocks.UdsClient.enableShutdownMode).toHaveBeenCalled();
        expect(mocks.UdsClient.shutdown).toHaveBeenCalled();
    });

    afterEach(() => {
        jest.clearAllMocks();
    });
});
