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
const UtilInternal = require("../../../../main/js/joynr/util/UtilInternal");

const provisioning_root = require("../../../resources/joynr/provisioning/provisioning_root"); // logger and mqtt
provisioning_root.ccAddress = {
    protocol: "ws",
    host: "localhost",
    port: 4242,
    path: ""
};

const DiscoveryQos = require("../../../../main/js/joynr/proxy/DiscoveryQos");
const CapabilitiesRegistrar = require("../../../../main/js/joynr/capabilities/CapabilitiesRegistrar");
const MessageRouter = require("../../../../main/js/joynr/messaging/routing/MessageRouter");
const MessageQueue = require("../../../../main/js/joynr/messaging/routing/MessageQueue");
const Dispatcher = require("../../../../main/js/joynr/dispatching/Dispatcher");
const ParticipantIdStorage = require("../../../../main/js/joynr/capabilities/ParticipantIdStorage");
const PublicationManager = require("../../../../main/js/joynr/dispatching/subscription/PublicationManager");
const LocalStorage = require("../../../../main/js/global/LocalStorageNode");
const MessagingQos = require("../../../../main/js/joynr/messaging/MessagingQos");
const WebSocketMessagingSkeleton = require("../../../../main/js/joynr/messaging/websocket/WebSocketMessagingSkeleton");
const SharedWebSocket = require("../../../../main/js/joynr/messaging/websocket/SharedWebSocket");
const WebSocketMessagingStubFactory = require("../../../../main/js/joynr/messaging/websocket/WebSocketMessagingStubFactory");
const JoynrMessage = require("../../../../main/js/joynr/messaging/JoynrMessage");
const SubscriptionManager = require("../../../../main/js/joynr/dispatching/subscription/SubscriptionManager");

/**
 * this function creates a wrapper class, which calls Class not by its normal constructor but by Class.prototype.constructor
 * that way it's possible to spy on the constructor because we have access to its parent Object.
 * @param Class
 * @param {Function} fixClass
 *      this function is used to create additional spys after the constructor call because the class methods in joynr
 *      are often only created in the constructor.
 * @returns {wrappedClass}
 */
function wrapClass(Class, fixClass) {
    const wrappedClass = function() {
        Class.prototype.constructor.apply(this, arguments);
        // fixClass is a function which shall be called after the constructor because in some cases functions are declared
        // in the constructor ...
        if (fixClass) {
            fixClass.call(this);
        }
    };
    // only necessary if there is prototpye manipulation
    wrappedClass.prototype = Object.create(Class.prototype);
    // makes it so that our type checks work -> because they check for .prototype.constructor.name
    wrappedClass.prototype.constructor = Class;
    return wrappedClass;
}

function fixMessageRouter() {
    spyOn(this, "setRoutingProxy").and.returnValue(Promise.resolve());
}

let terminateSubscriptionsSpy;
function fixSubScriptionManager() {
    terminateSubscriptionsSpy = spyOn(this, "terminateSubscriptions").and.returnValue(Promise.resolve());
}

const mocks = {};

mocks.MessageRouterMock = wrapClass(MessageRouter, fixMessageRouter);
mocks.SubscriptionManagerMock = wrapClass(SubscriptionManager, fixSubScriptionManager);
mocks.MessageQueueMock = wrapClass(MessageQueue);
mocks.DispatcherMock = wrapClass(Dispatcher);
mocks.ParticipantIdStorageMock = wrapClass(ParticipantIdStorage);
mocks.PublicationManagerMock = wrapClass(PublicationManager);
mocks.MessagingQosMock = wrapClass(MessagingQos);
mocks.WebSocketMessagingSkeletonMock = wrapClass(WebSocketMessagingSkeleton);
mocks.SharedWebSocketMock = wrapClass(SharedWebSocket);
mocks.WebSocketMessagingStubFactoryMock = wrapClass(WebSocketMessagingStubFactory);

const mod = require("module");

const savedRequire = mod.prototype.require;
mod.prototype.require = function(md) {
    const index = md.lastIndexOf("/") + 1;
    const mock = `${md.substr(index)}Mock`;

    if (mocks[mock]) {
        return mocks[mock];
    }
    return savedRequire.apply(this, arguments);
};

const WebSocketLibjoynrRuntime = require("../../../../main/js/joynr/start/WebSocketLibjoynrRuntime");

// restore old require for other tests after requireing dependencies
mod.prototype.require = savedRequire;

describe("libjoynr-js.joynr.start.WebSocketLibjoynrRuntime", () => {
    beforeAll(() => {
        // Since require Objects are cached we get the same instance as WebSocketLibjoynrRuntime
        // and can freely manipulate it before the new WebSocketLibjoynrRuntime call.
        // But after all tests we have to make sure to clear the cache of the required objects.
        spyOn(DiscoveryQos, "setDefaultSettings").and.callThrough();
        spyOn(CapabilitiesRegistrar, "setDefaultExpiryIntervalMs").and.callThrough();
        // unfortunately this does not involve constructors. In order to mock constructors
        // we have to mock the require call directly.

        spyOn(MessageRouter.prototype, "constructor").and.callThrough();
        spyOn(MessageQueue.prototype, "constructor").and.callThrough();
        spyOn(Dispatcher.prototype, "constructor").and.callThrough();
        spyOn(ParticipantIdStorage.prototype, "constructor").and.callThrough();
        spyOn(PublicationManager.prototype, "constructor").and.callThrough();
        spyOn(MessagingQos.prototype, "constructor").and.callThrough();
        spyOn(WebSocketMessagingSkeleton.prototype, "constructor").and.callThrough();
        spyOn(SharedWebSocket.prototype, "constructor").and.callThrough();
        spyOn(WebSocketMessagingStubFactory.prototype, "constructor").and.callThrough();
    });

    let runtime;
    let provisioning;

    beforeEach(done => {
        MessageRouter.prototype.constructor.calls.reset();
        MessageQueue.prototype.constructor.calls.reset();
        Dispatcher.prototype.constructor.calls.reset();
        ParticipantIdStorage.prototype.constructor.calls.reset();
        PublicationManager.prototype.constructor.calls.reset();
        MessagingQos.prototype.constructor.calls.reset();
        WebSocketMessagingSkeleton.prototype.constructor.calls.reset();
        SharedWebSocket.prototype.constructor.calls.reset();
        WebSocketMessagingStubFactory.prototype.constructor.calls.reset();

        provisioning = UtilInternal.extend({}, provisioning_root);
        done();
    });

    it("won't override settings unnecessarily", async () => {
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        await runtime.shutdown();
        expect(DiscoveryQos.setDefaultSettings).not.toHaveBeenCalled();
        expect(CapabilitiesRegistrar.setDefaultExpiryIntervalMs).not.toHaveBeenCalled();
    });

    it("will set the default discoveryQos settings correctly", async () => {
        const discoveryRetryDelayMs = 100;
        const discoveryTimeoutMs = 200;
        const discoveryExpiryIntervalMs = 100;
        const discoveryQos = {
            discoveryRetryDelayMs,
            discoveryTimeoutMs,
            discoveryExpiryIntervalMs
        };
        provisioning.discoveryQos = discoveryQos;
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        await runtime.shutdown();

        expect(DiscoveryQos.setDefaultSettings).toHaveBeenCalledWith({
            discoveryRetryDelayMs,
            discoveryTimeoutMs
        });
        expect(CapabilitiesRegistrar.setDefaultExpiryIntervalMs).toHaveBeenCalledWith(discoveryExpiryIntervalMs);
    });

    it("will initialize SharedWebSocket correctly", async () => {
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);

        expect(SharedWebSocket.prototype.constructor).toHaveBeenCalledWith({
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
        expect(MessageRouter.prototype.constructor.calls.count()).toEqual(1);
        expect(MessageRouter.prototype.constructor.calls.argsFor(0)[0].persistency).toBeUndefined();
        expect(ParticipantIdStorage.prototype.constructor.calls.count()).toEqual(1);
        expect(ParticipantIdStorage.prototype.constructor.calls.argsFor(0)[0]).toEqual(jasmine.any(LocalStorage));
        expect(PublicationManager.prototype.constructor.calls.count()).toEqual(1);
        expect(PublicationManager.prototype.constructor.calls.argsFor(0)[1]).toEqual(jasmine.any(LocalStorage));
    });

    it("enables MessageRouter Persistency if configured", async () => {
        provisioning.persistency = { routingTable: true };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(MessageRouter.prototype.constructor.calls.count()).toEqual(1);
        expect(MessageRouter.prototype.constructor.calls.argsFor(0)[0].persistency).toEqual(jasmine.any(LocalStorage));
        expect(MessageRouter.prototype.constructor).toHaveBeenCalledWith(
            jasmine.objectContaining({ persistency: jasmine.any(Object) })
        );
    });

    it("enables ParticipantIdStorage persistency if configured", async () => {
        provisioning.persistency = { capabilities: true };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(ParticipantIdStorage.prototype.constructor.calls.count()).toEqual(1);
        expect(ParticipantIdStorage.prototype.constructor.calls.argsFor(0)[0]).toEqual(jasmine.any(LocalStorage));
    });

    it("disables PublicationManager persistency if configured", async () => {
        provisioning.persistency = { publications: false };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(PublicationManager.prototype.constructor.calls.count()).toEqual(1);
        expect(PublicationManager.prototype.constructor.calls.argsFor(0)[1]).toBeUndefined();
    });

    it("will call MessageQueue with the settings from the provisioning", async () => {
        const maxQueueSizeInKBytes = 100;
        provisioning.messaging = { maxQueueSizeInKBytes };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(MessageQueue.prototype.constructor.calls.count()).toEqual(1);
        expect(MessageQueue.prototype.constructor).toHaveBeenCalledWith({
            maxQueueSizeInKBytes
        });
    });

    it("will call Dispatcher with the settings from the provisioning", async () => {
        const ttlUpLiftMs = 1000;
        provisioning.messaging = { TTL_UPLIFT: ttlUpLiftMs };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(Dispatcher.prototype.constructor.calls.count()).toEqual(1);
        expect(Dispatcher.prototype.constructor.calls.argsFor(0)[2]).toEqual(ttlUpLiftMs);
    });

    it("will call MessagingQos with the settings from the provisioning", async () => {
        const ttl = 1000;
        provisioning.internalMessagingQos = { ttl };
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        expect(MessagingQos.prototype.constructor).toHaveBeenCalledWith({ ttl });
    });

    it("will set the signingCallback to the joynrMessage.prototype", async () => {
        provisioning.keychain = {
            tlsCert: "tlsCert",
            tlsKey: "tlsKey",
            tlsCa: "tlsCa",
            ownerId: "ownerID"
        };
        spyOn(JoynrMessage, "setSigningCallback").and.callThrough();
        runtime = new WebSocketLibjoynrRuntime();
        await runtime.start(provisioning);
        await runtime.shutdown();
        expect(JoynrMessage.setSigningCallback).toHaveBeenCalled();
        const joynrMessage = new JoynrMessage({ payload: "payload", type: "type" });
        expect(joynrMessage.signingCallback()).toEqual(Buffer.from(provisioning.keychain.ownerId));
    });

    it("terminates Subscriptions upon shutdown with default timeout", done => {
        runtime = new WebSocketLibjoynrRuntime();
        runtime
            .start(provisioning)
            .then(runtime.shutdown)
            .then(() => {
                expect(terminateSubscriptionsSpy).toHaveBeenCalledWith(1000);
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
                runtime.shutdown();
            })
            .then(() => {
                expect(terminateSubscriptionsSpy).not.toHaveBeenCalled();
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
                expect(terminateSubscriptionsSpy).not.toHaveBeenCalled();
                done();
            })
            .catch(fail);
    });
});
