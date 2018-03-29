/*jslint es5: true, nomen: true, node: true */
/*global fail: true, beforeAll: true */

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
var Promise = require("../../../classes/global/Promise");
var Util = require("../../../classes/joynr/util/UtilInternal");

var provisioning_root = require("../../../test-classes/joynr/provisioning/provisioning_root"); // logger and mqtt
provisioning_root.ccAddress = {
    protocol: "ws",
    host: "localhost",
    port: 4242,
    path: ""
};

var DiscoveryQos = require("../../../classes/joynr/proxy/DiscoveryQos");
var CapabilitiesRegistrar = require("../../../classes/joynr/capabilities/CapabilitiesRegistrar");
var MessageRouter = require("../../../classes/joynr/messaging/routing/MessageRouter");
var MessageQueue = require("../../../classes/joynr/messaging/routing/MessageQueue");
var Dispatcher = require("../../../classes/joynr/dispatching/Dispatcher");
var ParticipantIdStorage = require("../../../classes/joynr/capabilities/ParticipantIdStorage");
var MemoryStorage = require("../../../classes/global/MemoryStorage");
var PublicationManager = require("../../../classes/joynr/dispatching/subscription/PublicationManager");
var LocalStorage = require("../../../classes/global/LocalStorageNode");
var MessagingQos = require("../../../classes/joynr/messaging/MessagingQos");
var WebSocketMessagingSkeleton = require("../../../classes/joynr/messaging/websocket/WebSocketMessagingSkeleton");
var SharedWebSocket = require("../../../classes/joynr/messaging/websocket/SharedWebSocket");
var WebSocketMessagingStubFactory = require("../../../classes/joynr/messaging/websocket/WebSocketMessagingStubFactory");
var JoynrMessage = require("../../../classes/joynr/messaging/JoynrMessage");

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
    var wrappedClass = function() {
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

var setRoutingProxySpy;
function fixMessageRouter() {
    setRoutingProxySpy = spyOn(this, "setRoutingProxy").and.returnValue(Promise.resolve());
}

var mocks = {};

mocks.MessageRouterMock = wrapClass(MessageRouter, fixMessageRouter);
mocks.MessageQueueMock = wrapClass(MessageQueue);
mocks.DispatcherMock = wrapClass(Dispatcher);
mocks.ParticipantIdStorageMock = wrapClass(ParticipantIdStorage);
mocks.PublicationManagerMock = wrapClass(PublicationManager);
mocks.MessagingQosMock = wrapClass(MessagingQos);
mocks.WebSocketMessagingSkeletonMock = wrapClass(WebSocketMessagingSkeleton);
mocks.SharedWebSocketMock = wrapClass(SharedWebSocket);
mocks.WebSocketMessagingStubFactoryMock = wrapClass(WebSocketMessagingStubFactory);

var mod = require("module");

var savedRequire = mod.prototype.require;
mod.prototype.require = function(md) {
    var index = md.lastIndexOf("/") + 1;
    var mock = md.substr(index) + "Mock";

    if (mocks[mock]) {
        return mocks[mock];
    }
    return savedRequire.apply(this, arguments);
};

var WebSocketLibjoynrRuntime = require("../../../classes/joynr/start/WebSocketLibjoynrRuntime");

// restore old require for other tests after requireing dependencies
mod.prototype.require = savedRequire;

describe("libjoynr-js.joynr.start.WebSocketLibjoynrRuntime", function() {
    beforeAll(function() {
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

    var runtime;
    var provisioning;

    beforeEach(function(done) {
        MessageRouter.prototype.constructor.calls.reset();
        MessageQueue.prototype.constructor.calls.reset();
        Dispatcher.prototype.constructor.calls.reset();
        ParticipantIdStorage.prototype.constructor.calls.reset();
        PublicationManager.prototype.constructor.calls.reset();
        MessagingQos.prototype.constructor.calls.reset();
        WebSocketMessagingSkeleton.prototype.constructor.calls.reset();
        SharedWebSocket.prototype.constructor.calls.reset();
        WebSocketMessagingStubFactory.prototype.constructor.calls.reset();

        provisioning = Util.extend({}, provisioning_root);
        done();
    });

    it("won't override settings unnecessarily", function() {
        runtime = new WebSocketLibjoynrRuntime(provisioning);
        expect(DiscoveryQos.setDefaultSettings).not.toHaveBeenCalled();
        expect(CapabilitiesRegistrar.setDefaultExpiryIntervalMs).not.toHaveBeenCalled();
    });

    it("will set the default discoveryQos settings correctly", function() {
        var discoveryRetryDelayMs = 100,
            discoveryTimeoutMs = 200,
            discoveryExpiryIntervalMs = 100;
        var discoveryQos = {
            discoveryRetryDelayMs: discoveryRetryDelayMs,
            discoveryTimeoutMs: discoveryTimeoutMs,
            discoveryExpiryIntervalMs: discoveryExpiryIntervalMs
        };
        provisioning.capabilities = { discoveryQos: discoveryQos };
        runtime = new WebSocketLibjoynrRuntime(provisioning);
        expect(DiscoveryQos.setDefaultSettings).toHaveBeenCalledWith({
            discoveryRetryDelayMs: discoveryRetryDelayMs,
            discoveryTimeoutMs: discoveryTimeoutMs
        });
        expect(CapabilitiesRegistrar.setDefaultExpiryIntervalMs).toHaveBeenCalledWith(discoveryExpiryIntervalMs);
    });

    it("will initialize SharedWebSocket correctly", function(done) {
        runtime = new WebSocketLibjoynrRuntime(provisioning);
        runtime
            .start()
            .then(done)
            .catch(fail);

        expect(SharedWebSocket.prototype.constructor).toHaveBeenCalledWith({
            remoteAddress: jasmine.objectContaining({
                _typeName: "joynr.system.RoutingTypes.WebSocketAddress",
                protocol: provisioning.ccAddress.protocol,
                host: provisioning.ccAddress.host,
                port: provisioning.ccAddress.port,
                path: provisioning.ccAddress.path
            }),
            localAddress: jasmine.objectContaining({ _typeName: "joynr.system.RoutingTypes.WebSocketClientAddress" }),
            provisioning: jasmine.any(Object),
            keychain: undefined
        });
    });

    it("will use the default persistency settings", function(done) {
        runtime = new WebSocketLibjoynrRuntime(provisioning);
        runtime
            .start()
            .then(done)
            .catch(fail);
        expect(MessageRouter.prototype.constructor.calls.count()).toEqual(1);
        expect(MessageRouter.prototype.constructor.calls.argsFor(0)[0].persistency).toBeUndefined();
        expect(ParticipantIdStorage.prototype.constructor.calls.count()).toEqual(1);
        expect(ParticipantIdStorage.prototype.constructor.calls.argsFor(0)[0]).toEqual(jasmine.any(LocalStorage));
        expect(PublicationManager.prototype.constructor.calls.count()).toEqual(1);
        expect(PublicationManager.prototype.constructor.calls.argsFor(0)[1]).toEqual(jasmine.any(LocalStorage));
    });

    it("enables MessageRouter Persistency if configured", function(done) {
        provisioning.persistency = { routingTable: true };
        runtime = new WebSocketLibjoynrRuntime(provisioning);
        runtime
            .start()
            .then(done)
            .catch(fail);
        expect(MessageRouter.prototype.constructor.calls.count()).toEqual(1);
        expect(MessageRouter.prototype.constructor.calls.argsFor(0)[0].persistency).toEqual(jasmine.any(LocalStorage));
        expect(MessageRouter.prototype.constructor).toHaveBeenCalledWith(
            jasmine.objectContaining({ persistency: jasmine.any(Object) })
        );
    });

    it("enables ParticipantIdStorage persistency if configured", function(done) {
        provisioning.persistency = { capabilities: true };
        runtime = new WebSocketLibjoynrRuntime(provisioning);
        runtime
            .start()
            .then(done)
            .catch(fail);
        expect(ParticipantIdStorage.prototype.constructor.calls.count()).toEqual(1);
        expect(ParticipantIdStorage.prototype.constructor.calls.argsFor(0)[0]).toEqual(jasmine.any(LocalStorage));
    });

    it("disables PublicationManager persistency if configured", function(done) {
        provisioning.persistency = { publications: false };
        runtime = new WebSocketLibjoynrRuntime(provisioning);
        runtime
            .start()
            .then(done)
            .catch(fail);
        expect(PublicationManager.prototype.constructor.calls.count()).toEqual(1);
        expect(PublicationManager.prototype.constructor.calls.argsFor(0)[1]).toBeUndefined();
    });

    it("will call MessageQueue with the settings from the provisioning", function(done) {
        var maxQueueSizeInKBytes = 100;
        provisioning.messaging = { maxQueueSizeInKBytes: maxQueueSizeInKBytes };
        runtime = new WebSocketLibjoynrRuntime(provisioning);
        runtime
            .start()
            .then(done)
            .catch(fail);
        expect(MessageQueue.prototype.constructor.calls.count()).toEqual(1);
        expect(MessageQueue.prototype.constructor).toHaveBeenCalledWith({ maxQueueSizeInKBytes: maxQueueSizeInKBytes });
    });

    it("will call Dispatcher with the settings from the provisioning", function(done) {
        var ttlUpLiftMs = 1000;
        provisioning.messaging = { TTL_UPLIFT: ttlUpLiftMs };
        runtime = new WebSocketLibjoynrRuntime(provisioning);
        runtime
            .start()
            .then(done)
            .catch(fail);
        expect(Dispatcher.prototype.constructor.calls.count()).toEqual(1);
        expect(Dispatcher.prototype.constructor.calls.argsFor(0)[2]).toEqual(ttlUpLiftMs);
    });

    it("will call MessagingQos with the settings from the provisioning", function(done) {
        var ttl = 1000;
        provisioning.internalMessagingQos = { ttl: ttl };
        runtime = new WebSocketLibjoynrRuntime(provisioning);
        runtime
            .start()
            .then(done)
            .catch(fail);
        expect(MessagingQos.prototype.constructor).toHaveBeenCalledWith({ ttl: ttl });
    });

    it("will set the signingCallback to the joynrMessage.prototype", function() {
        provisioning.keychain = {
            tlsCert: "tlsCert",
            tlsKey: "tlsKey",
            tlsCa: "tlsCa",
            ownerId: "ownerID"
        };
        spyOn(JoynrMessage, "setSigningCallback").and.callThrough();
        runtime = new WebSocketLibjoynrRuntime(provisioning);
        expect(JoynrMessage.setSigningCallback).toHaveBeenCalled();
        var joynrMessage = new JoynrMessage({ payload: "payload", type: "type" });
        expect(joynrMessage.signingCallback()).toEqual(Buffer.from(provisioning.keychain.ownerId));
    });
});
