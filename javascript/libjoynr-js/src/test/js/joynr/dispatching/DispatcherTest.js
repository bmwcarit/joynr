/* eslint prefer-promise-reject-errors: "off" */
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
require("../../node-unit-test-helper");
const Dispatcher = require("../../../../main/js/joynr/dispatching/Dispatcher");
const JoynrMessage = require("../../../../main/js/joynr/messaging/JoynrMessage");
const MessagingQos = require("../../../../main/js/joynr/messaging/MessagingQos");
const MessagingQosEffort = require("../../../../main/js/joynr/messaging/MessagingQosEffort");
const OneWayRequest = require("../../../../main/js/joynr/dispatching/types/OneWayRequest");
const Request = require("../../../../main/js/joynr/dispatching/types/Request");
const Reply = require("../../../../main/js/joynr/dispatching/types/Reply");
const BroadcastSubscriptionRequest = require("../../../../main/js/joynr/dispatching/types/BroadcastSubscriptionRequest");
const MulticastSubscriptionRequest = require("../../../../main/js/joynr/dispatching/types/MulticastSubscriptionRequest");
const SubscriptionRequest = require("../../../../main/js/joynr/dispatching/types/SubscriptionRequest");
const SubscriptionReply = require("../../../../main/js/joynr/dispatching/types/SubscriptionReply");
const SubscriptionStop = require("../../../../main/js/joynr/dispatching/types/SubscriptionStop");
const MulticastPublication = require("../../../../main/js/joynr/dispatching/types/MulticastPublication");
const SubscriptionPublication = require("../../../../main/js/joynr/dispatching/types/SubscriptionPublication");
const TypeRegistrySingleton = require("../../../../main/js/joynr/types/TypeRegistrySingleton");
const DiscoveryEntryWithMetaInfo = require("../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo");
const Version = require("../../../../main/js/generated/joynr/types/Version");
const ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");
const nanoid = require("nanoid");
const LoggingManager = require("../../../../main/js/joynr/system/LoggingManager");

const providerId = "providerId";
const proxyId = "proxyId";
let toDiscoveryEntry, globalToDiscoveryEntry;

describe("libjoynr-js.joynr.dispatching.Dispatcher", () => {
    let dispatcher,
        requestReplyManager,
        subscriptionManager,
        publicationManager,
        messageRouter,
        clusterControllerMessagingStub,
        securityManager;
    const subscriptionId = `mySubscriptionId-${nanoid()}`;
    const multicastId = `multicastId-${nanoid()}`;
    const requestReplyId = "requestReplyId";
    let loggerSpy;

    beforeAll(() => {
        spyOn(LoggingManager, "getLogger").and.callThrough();
    });

    /**
     * Called before each test.
     */
    beforeEach(done => {
        toDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
            domain: "testProviderDomain",
            interfaceName: "interfaceName",
            participantId: providerId,
            qos: new ProviderQos(),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "publicKeyId",
            isLocal: true
        });
        globalToDiscoveryEntry = new DiscoveryEntryWithMetaInfo(toDiscoveryEntry);
        globalToDiscoveryEntry.isLocal = false;
        requestReplyManager = jasmine.createSpyObj("RequestReplyManager", [
            "handleOneWayRequest",
            "handleRequest",
            "handleReply"
        ]);
        subscriptionManager = jasmine.createSpyObj("SubscriptionManager", [
            "handleSubscriptionReply",
            "handleMulticastPublication",
            "handlePublication"
        ]);
        const sendSubscriptionReply = function(
            proxyParticipantId,
            providerParticipantId,
            subscriptionRequest,
            callbackDispatcher,
            callbackDispatcherSettings
        ) {
            callbackDispatcher(
                callbackDispatcherSettings,
                new SubscriptionReply({
                    subscriptionId: subscriptionRequest.subscriptionId
                })
            );
        };

        publicationManager = {
            handleSubscriptionRequest: sendSubscriptionReply,
            handleMulticastSubscriptionRequest: sendSubscriptionReply,
            handleSubscriptionStop() {}
        };
        spyOn(publicationManager, "handleSubscriptionRequest").and.callThrough();
        spyOn(publicationManager, "handleMulticastSubscriptionRequest").and.callThrough();
        spyOn(publicationManager, "handleSubscriptionStop");

        messageRouter = jasmine.createSpyObj("MessageRouter", ["addMulticastReceiver", "removeMulticastReceiver"]);
        messageRouter.addMulticastReceiver.and.returnValue(Promise.resolve());
        clusterControllerMessagingStub = jasmine.createSpyObj("ClusterControllerMessagingStub", ["transmit"]);
        clusterControllerMessagingStub.transmit.and.returnValue(Promise.resolve());

        securityManager = jasmine.createSpyObj("SecurityManager", ["getCurrentProcessUserId"]);

        dispatcher = new Dispatcher(clusterControllerMessagingStub, securityManager);
        dispatcher.registerRequestReplyManager(requestReplyManager);
        dispatcher.registerSubscriptionManager(subscriptionManager);
        dispatcher.registerPublicationManager(publicationManager);
        dispatcher.registerMessageRouter(messageRouter);

        loggerSpy = LoggingManager.getLogger.calls.mostRecent().returnValue;
        spyOn(loggerSpy, "error");

        /*
         * Make sure 'TestEnum' is properly registered as a type.
         * Just requiring the module is insufficient since the
         * automatically generated code called async methods.
         * Execution might be still in progress.
         */
        TypeRegistrySingleton.getInstance()
            .getTypeRegisteredPromise("joynr.tests.testTypes.TestEnum", 1000)
            .then(() => {
                done();
                return null;
            })
            .catch(fail);
    });

    it("is instantiable and of correct type", done => {
        expect(Dispatcher).toBeDefined();
        expect(typeof Dispatcher === "function").toBeTruthy();
        expect(dispatcher).toBeDefined();
        expect(dispatcher instanceof Dispatcher).toBeTruthy();
        expect(dispatcher.registerRequestReplyManager).toBeDefined();
        expect(typeof dispatcher.registerRequestReplyManager === "function").toBeTruthy();
        expect(dispatcher.registerSubscriptionManager).toBeDefined();
        expect(typeof dispatcher.registerSubscriptionManager === "function").toBeTruthy();
        expect(dispatcher.registerPublicationManager).toBeDefined();
        expect(typeof dispatcher.registerPublicationManager === "function").toBeTruthy();
        expect(dispatcher.sendRequest).toBeDefined();
        expect(typeof dispatcher.sendRequest === "function").toBeTruthy();
        expect(dispatcher.sendOneWayRequest).toBeDefined();
        expect(typeof dispatcher.sendOneWayRequest === "function").toBeTruthy();
        expect(dispatcher.sendBroadcastSubscriptionRequest).toBeDefined();
        expect(typeof dispatcher.sendBroadcastSubscriptionRequest === "function").toBeTruthy();
        expect(dispatcher.sendSubscriptionRequest).toBeDefined();
        expect(typeof dispatcher.sendSubscriptionRequest === "function").toBeTruthy();
        expect(dispatcher.sendSubscriptionStop).toBeDefined();
        expect(typeof dispatcher.sendSubscriptionStop === "function").toBeTruthy();
        expect(dispatcher.sendPublication).toBeDefined();
        expect(typeof dispatcher.sendPublication === "function").toBeTruthy();
        expect(dispatcher.receive).toBeDefined();
        expect(typeof dispatcher.receive === "function").toBeTruthy();
        done();
    });

    function receiveJoynrMessage(parameters) {
        const joynrMessage = new JoynrMessage({
            type: parameters.type,
            payload: JSON.stringify(parameters.payload)
        });
        joynrMessage.from = proxyId;
        joynrMessage.to = providerId;
        dispatcher.receive(joynrMessage);
    }

    it("forwards subscription request to Publication Manager", () => {
        const payload = {
            subscribedToName: "attributeName",
            subscriptionId
        };
        receiveJoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
            payload
        });
        expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalled();
        expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalledWith(
            proxyId,
            providerId,
            new SubscriptionRequest(payload),
            jasmine.any(Function),
            jasmine.any(Object)
        );
    });

    it("forwards multicast subscription request to Publication Manager", () => {
        const payload = {
            subscribedToName: "multicastEvent",
            subscriptionId,
            multicastId
        };
        receiveJoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST,
            payload
        });
        expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalled();
        expect(publicationManager.handleMulticastSubscriptionRequest).toHaveBeenCalledWith(
            proxyId,
            providerId,
            new MulticastSubscriptionRequest(payload),
            jasmine.any(Function),
            jasmine.any(Object)
        );
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
        expect(sentMessage.payload).toMatch(
            `{"subscriptionId":"${subscriptionId}","_typeName":"joynr.SubscriptionReply"}`
        );
    });

    it("forwards subscription reply to Subscription Manager", done => {
        const payload = {
            subscriptionId
        };
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY,
            payload: JSON.stringify(payload)
        });
        dispatcher.receive(joynrMessage);
        expect(subscriptionManager.handleSubscriptionReply).toHaveBeenCalled();
        expect(subscriptionManager.handleSubscriptionReply).toHaveBeenCalledWith(new SubscriptionReply(payload));
        done();
    });

    it("forwards subscription stop to SubscriptionPublication Manager", done => {
        const payload = {
            subscriptionId
        };
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_STOP,
            payload: JSON.stringify(payload)
        });
        dispatcher.receive(joynrMessage);
        expect(publicationManager.handleSubscriptionStop).toHaveBeenCalled();
        expect(publicationManager.handleSubscriptionStop).toHaveBeenCalledWith(new SubscriptionStop(payload));
        done();
    });

    function receivePublication(parameters) {
        const joynrMessage = new JoynrMessage({
            type: parameters.type,
            payload: JSON.stringify(parameters.payload)
        });
        dispatcher.receive(joynrMessage);
    }

    it("forwards publication to Subscription Manager", done => {
        const payload = {
            subscriptionId,
            response: "myResponse"
        };
        receivePublication({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_PUBLICATION,
            payload
        });
        expect(subscriptionManager.handlePublication).toHaveBeenCalled();
        expect(subscriptionManager.handlePublication).toHaveBeenCalledWith(SubscriptionPublication.create(payload));
        done();
    });

    it("forwards multicast publication to Subscription Manager", done => {
        const payload = {
            multicastId,
            response: "myResponse"
        };
        receivePublication({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST,
            payload
        });
        expect(subscriptionManager.handleMulticastPublication).toHaveBeenCalled();
        expect(subscriptionManager.handleMulticastPublication).toHaveBeenCalledWith(
            MulticastPublication.create(payload)
        );
        done();
    });

    it("forwards request to RequestReply Manager", done => {
        const request = Request.create({
            methodName: "methodName"
        });
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: JSON.stringify(request)
        });
        joynrMessage.to = providerId;
        joynrMessage.from = proxyId;
        dispatcher.receive(joynrMessage);
        expect(requestReplyManager.handleRequest).toHaveBeenCalled();
        expect(requestReplyManager.handleRequest.calls.mostRecent().args[0]).toEqual(providerId);
        expect(requestReplyManager.handleRequest.calls.mostRecent().args[1]).toEqual(request);
        done();
    });

    it("forwards one-way request to RequestReply Manager", done => {
        const oneWayRequest = OneWayRequest.create({
            methodName: "methodName"
        });
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_ONE_WAY,
            payload: JSON.stringify(oneWayRequest)
        });
        joynrMessage.to = providerId;
        joynrMessage.from = proxyId;
        dispatcher.receive(joynrMessage);
        expect(requestReplyManager.handleOneWayRequest).toHaveBeenCalled();
        expect(requestReplyManager.handleOneWayRequest.calls.mostRecent().args[0]).toEqual(providerId);
        expect(requestReplyManager.handleOneWayRequest.calls.mostRecent().args[1]).toEqual(oneWayRequest);
        done();
    });

    it("forwards reply to RequestReply Manager", done => {
        const reply = Reply.create({
            requestReplyId,
            response: []
        });
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REPLY,
            payload: JSON.stringify(reply)
        });
        dispatcher.receive(joynrMessage);
        expect(requestReplyManager.handleReply).toHaveBeenCalled();
        expect(requestReplyManager.handleReply).toHaveBeenCalledWith(reply);
        done();
    });

    function sendBroadcastSubscriptionRequest(request) {
        const messagingQos = new MessagingQos();
        return dispatcher.sendBroadcastSubscriptionRequest({
            from: proxyId,
            toDiscoveryEntry,
            messagingQos,
            subscriptionRequest: request
        });
    }

    it("is able to send multicast subscription request", done => {
        const multicastId = "multicastId";
        const multicastSubscriptionRequest = new MulticastSubscriptionRequest({
            subscribedToName: "multicastEvent",
            subscriptionId: "subscriptionId",
            multicastId
        });
        const serializedPayload = JSON.stringify(multicastSubscriptionRequest);

        expect(messageRouter.addMulticastReceiver).not.toHaveBeenCalled();
        expect(clusterControllerMessagingStub.transmit).not.toHaveBeenCalled();

        sendBroadcastSubscriptionRequest(multicastSubscriptionRequest)
            .then(() => {
                expect(messageRouter.addMulticastReceiver).toHaveBeenCalledTimes(1);
                const addMulticastReceiverParams = messageRouter.addMulticastReceiver.calls.argsFor(0)[0];
                expect(addMulticastReceiverParams.multicastId).toEqual(multicastId);
                expect(addMulticastReceiverParams.subscriberParticipantId).toEqual(proxyId);
                expect(addMulticastReceiverParams.providerParticipantId).toEqual(providerId);

                expect(clusterControllerMessagingStub.transmit).toHaveBeenCalledTimes(1);
                const sentMessage = clusterControllerMessagingStub.transmit.calls.argsFor(0)[0];
                expect(sentMessage.type).toEqual(JoynrMessage.JOYNRMESSAGE_TYPE_MULTICAST_SUBSCRIPTION_REQUEST);
                expect(sentMessage.from).toEqual(proxyId);
                expect(sentMessage.to).toEqual(providerId);
                expect(sentMessage.payload).toEqual(serializedPayload);

                done();
            })
            .catch(done.fail);
    });

    it("does not send multicast subscription request if addMulticastReceiver fails", done => {
        const multicastId = "multicastId";
        const multicastSubscriptionRequest = new MulticastSubscriptionRequest({
            subscribedToName: "multicastEvent",
            subscriptionId: "subscriptionId",
            multicastId
        });

        messageRouter.addMulticastReceiver.and.returnValue(Promise.reject());
        expect(messageRouter.addMulticastReceiver).not.toHaveBeenCalled();
        expect(clusterControllerMessagingStub.transmit).not.toHaveBeenCalled();

        sendBroadcastSubscriptionRequest(multicastSubscriptionRequest)
            .then(fail)
            .catch(() => {
                expect(messageRouter.addMulticastReceiver).toHaveBeenCalledTimes(1);
                expect(clusterControllerMessagingStub.transmit).not.toHaveBeenCalled();
                done();
            });
    });

    it("is able to send selective broadcast subscription request", () => {
        expect(clusterControllerMessagingStub.transmit).not.toHaveBeenCalled();
        const broadcastSubscriptionRequest = new BroadcastSubscriptionRequest({
            subscribedToName: "broadcastEvent",
            subscriptionId: "subscriptionId"
        });
        const serializedPayload = JSON.stringify(broadcastSubscriptionRequest);

        sendBroadcastSubscriptionRequest(broadcastSubscriptionRequest);

        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalledTimes(1);
        const sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
        expect(sentMessage.type).toEqual(JoynrMessage.JOYNRMESSAGE_TYPE_BROADCAST_SUBSCRIPTION_REQUEST);
        expect(sentMessage.from).toEqual(proxyId);
        expect(sentMessage.to).toEqual(providerId);
        expect(sentMessage.payload).toEqual(serializedPayload);
    });

    function setsIsLocalMessageInSubscriptionRequest(subscriptionRequest, sendMethod) {
        let sentMessage;
        const messagingQos = new MessagingQos();

        return sendMethod({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            subscriptionRequest
        })
            .then(() => {
                expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
                sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
                expect(sentMessage.isLocalMessage).toEqual(true);

                return sendMethod({
                    from: "from",
                    toDiscoveryEntry: globalToDiscoveryEntry,
                    messagingQos,
                    subscriptionRequest
                });
            })
            .then(() => {
                expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
                sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
                expect(sentMessage.isLocalMessage).toEqual(false);
            });
    }

    it("sets isLocalMessage in request messages", done => {
        let sentMessage;
        const messagingQos = new MessagingQos();

        const request = Request.create({
            methodName: "methodName"
        });
        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
        expect(sentMessage.isLocalMessage).toEqual(true);

        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry: globalToDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
        expect(sentMessage.isLocalMessage).toEqual(false);

        done();
    });

    it("sets isLocalMessage in subscription request messages", done => {
        const subscriptionRequestPayload = {
            subscribedToName: "subscribeToName",
            subscriptionId
        };
        const subscriptionRequest = new SubscriptionRequest(subscriptionRequestPayload);
        setsIsLocalMessageInSubscriptionRequest(subscriptionRequest, dispatcher.sendSubscriptionRequest)
            .then(() => {
                const broadcastSubscriptionRequest = new BroadcastSubscriptionRequest(subscriptionRequestPayload);
                return setsIsLocalMessageInSubscriptionRequest(
                    broadcastSubscriptionRequest,
                    dispatcher.sendBroadcastSubscriptionRequest
                );
            })
            .then(() => {
                subscriptionRequestPayload.multicastId = multicastId;
                const multicastSubscriptionRequest = new MulticastSubscriptionRequest(subscriptionRequestPayload);
                setsIsLocalMessageInSubscriptionRequest(
                    multicastSubscriptionRequest,
                    dispatcher.sendBroadcastSubscriptionRequest
                ).then(done);
            })
            .catch(done.fail);
    });

    it("sets compress in request messages", done => {
        let sentMessage;
        const messagingQos = new MessagingQos();
        messagingQos.compress = true;

        const request = Request.create({
            methodName: "methodName"
        });
        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
        expect(sentMessage.isLocalMessage).toEqual(true);

        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry: globalToDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
        expect(sentMessage.compress).toEqual(true);

        done();
    });

    function verifyEffortForReply(request, expectedEffort) {
        expect(requestReplyManager.handleRequest).toHaveBeenCalled();
        expect(requestReplyManager.handleRequest.calls.mostRecent().args[0]).toEqual(providerId);
        expect(requestReplyManager.handleRequest.calls.mostRecent().args[1]).toEqual(request);
        expect(clusterControllerMessagingStub.transmit.calls.mostRecent().args[0].effort).toEqual(expectedEffort);

        requestReplyManager.handleRequest.calls.reset();
        clusterControllerMessagingStub.transmit.calls.reset();
    }

    it("sets the correct effort for replies if the effort was set in the corresponding request", async () => {
        const request = Request.create({
            methodName: "methodName"
        });
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: JSON.stringify(request)
        });
        joynrMessage.to = providerId;
        joynrMessage.from = proxyId;
        joynrMessage.compress = true;

        requestReplyManager.handleRequest.and.callFake((to, request, cb, replySettings) => {
            cb(replySettings, request);
            return Promise.resolve();
        });

        joynrMessage.effort = undefined; // default
        await dispatcher.receive(joynrMessage);
        verifyEffortForReply(request, undefined);

        joynrMessage.effort = MessagingQosEffort.NORMAL.value;
        await dispatcher.receive(joynrMessage);
        verifyEffortForReply(request, undefined);

        joynrMessage.effort = MessagingQosEffort.BEST_EFFORT.value;
        await dispatcher.receive(joynrMessage);
        verifyEffortForReply(request, MessagingQosEffort.BEST_EFFORT.value);

        joynrMessage.effort = "INVALID_EFFORT";
        await dispatcher.receive(joynrMessage);
        verifyEffortForReply(request, undefined);
    });

    it("sets the compressed flag for replies if the request was compressed", done => {
        const request = Request.create({
            methodName: "methodName"
        });
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REQUEST,
            payload: JSON.stringify(request)
        });
        joynrMessage.to = providerId;
        joynrMessage.from = proxyId;
        joynrMessage.compress = true;

        requestReplyManager.handleRequest.and.callFake((to, request, cb, replySettings) => {
            cb(replySettings, request);
            return Promise.resolve();
        });

        dispatcher.receive(joynrMessage).then(() => {
            expect(requestReplyManager.handleRequest).toHaveBeenCalled();
            expect(requestReplyManager.handleRequest.calls.mostRecent().args[0]).toEqual(providerId);
            expect(requestReplyManager.handleRequest.calls.mostRecent().args[1]).toEqual(request);
            expect(clusterControllerMessagingStub.transmit.calls.mostRecent().args[0].compress).toBe(true);
            done();
        });
    });

    it("enriches requests with custom headers", done => {
        const request = Request.create({
            methodName: "methodName"
        });
        const messagingQos = new MessagingQos();
        const headerKey = "key";
        const headerValue = "value";
        messagingQos.putCustomMessageHeader(headerKey, headerValue);
        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
        expect(sentMessage.getCustomHeaders()[headerKey]).toEqual(headerValue);
        done();
    });

    it("enriches requests with effort header", done => {
        const request = Request.create({
            methodName: "methodName"
        });
        const messagingQos = new MessagingQos();
        messagingQos.effort = MessagingQosEffort.BEST_EFFORT;
        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
        expect(sentMessage.effort).toEqual(MessagingQosEffort.BEST_EFFORT.value);
        done();
    });

    it("enriches one way requests with custom headers", done => {
        const request = OneWayRequest.create({
            methodName: "methodName"
        });
        const messagingQos = new MessagingQos();
        const headerKey = "key";
        const headerValue = "value";
        messagingQos.putCustomMessageHeader(headerKey, headerValue);
        dispatcher.sendOneWayRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
        expect(sentMessage.getCustomHeaders()[headerKey]).toEqual(headerValue);
        done();
    });

    it("enriches replies with custom headers from request", done => {
        let sentReplyMessage;
        const request = Request.create({
            methodName: "methodName"
        });
        const messagingQos = new MessagingQos();
        const headerKey = "key";
        const headerValue = "value";
        messagingQos.putCustomMessageHeader(headerKey, headerValue);
        dispatcher.sendRequest({
            from: "from",
            toDiscoveryEntry,
            messagingQos,
            request
        });
        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentRequestMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
        const clusterControllerMessagingStubTransmitCallsCount = clusterControllerMessagingStub.transmit.calls.count();
        // get ready for an incoming request: when handleRequest is called, pass an empty reply back.
        requestReplyManager.handleRequest.and.callFake((to, request, cb, replySettings) => {
            cb(replySettings, request);
            return Promise.resolve();
        });
        // now simulate receiving the request message, as if it had been transmitted
        // this will be passed on to the mock requestReplyManager
        dispatcher.receive(sentRequestMessage).then(() => {
            sentReplyMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
            expect(clusterControllerMessagingStub.transmit.calls.count()).toBe(
                clusterControllerMessagingStubTransmitCallsCount + 1
            );
            expect(sentReplyMessage.getCustomHeaders()[headerKey]).toEqual(headerValue);
            done();
        });
    });

    it("sends subscription reply on subscription request", done => {
        const payload = {
            subscribedToName: "attributeName",
            subscriptionId
        };
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REQUEST,
            payload: JSON.stringify(payload)
        });
        joynrMessage.from = proxyId;
        joynrMessage.to = providerId;

        const subscriptionReply = new SubscriptionReply(payload);

        /*
         * The dispatcher.receive() based on the message type calls
         * publicationManager.handleSubscriptionRequest()
         * and hands over a callback that invokes sendSubscriptionReply().
         * The resulting message is finally sent out using
         * clusterControllerMessagingStub.transmit().
         */

        publicationManager.handleSubscriptionRequest.and.callFake(
            (proxyId, providerId, subscriptionRequest, callback, settings) => {
                callback(settings, subscriptionReply);
            }
        );

        dispatcher.receive(joynrMessage);

        /*
         * Note: We can directly expect stuff here only just because
         * the jasmine spies do not emulate async action which would
         * otherwise occur in real code.
         */
        expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalled();
        expect(publicationManager.handleSubscriptionRequest).toHaveBeenCalledWith(
            proxyId,
            providerId,
            new SubscriptionRequest(payload),
            jasmine.any(Function),
            jasmine.any(Object)
        );

        expect(clusterControllerMessagingStub.transmit).toHaveBeenCalled();
        const sentMessage = clusterControllerMessagingStub.transmit.calls.mostRecent().args[0];
        expect(sentMessage.type).toEqual(JoynrMessage.JOYNRMESSAGE_TYPE_SUBSCRIPTION_REPLY);
        expect(sentMessage.payload).toEqual(JSON.stringify(subscriptionReply));

        done();
    });

    it("accepts messages with Parse Errors", done => {
        loggerSpy.error.calls.reset();
        const joynrMessage = new JoynrMessage({
            type: JoynrMessage.JOYNRMESSAGE_TYPE_REPLY,
            payload: "invalidJSONPayload[/}"
        });
        dispatcher
            .receive(joynrMessage)
            .then(done)
            .catch(fail);
        const lastArgs = loggerSpy.error.calls.argsFor(0)[0];
        expect(lastArgs.indexOf(joynrMessage.payload) !== -1).toBe(true);
    });
});
