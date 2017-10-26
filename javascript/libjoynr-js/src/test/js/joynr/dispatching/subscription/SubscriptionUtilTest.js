/*jslint node: true */

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
var SubscriptionUtil = require("../../../../classes/joynr/dispatching/subscription/util/SubscriptionUtil");
var SubscriptionInformation = require("../../../../classes/joynr/dispatching/types/SubscriptionInformation");
var SubscriptionRequest = require("../../../../classes/joynr/dispatching/types/SubscriptionRequest");
var SubscriptionStop = require("../../../../classes/joynr/dispatching/types/SubscriptionStop");
var PeriodicSubscriptionQos = require("../../../../classes/joynr/proxy/PeriodicSubscriptionQos");
var OnChangeSubscriptionQos = require("../../../../classes/joynr/proxy/OnChangeSubscriptionQos");
var OnChangeWithKeepAliveSubscriptionQos = require("../../../../classes/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos");
var ProviderQos = require("../../../../classes/joynr/types/ProviderQos");
var uuid = require("../../../../classes/lib/uuid-annotated");
var LoggerFactory = require("../../../../classes/joynr/system/LoggerFactory");

describe("libjoynr-js.joynr.dispatching.subscription.types.SubscriptionUtil", function() {
    var proxyId;
    var providerId;
    var publicationManager;
    var dispatcherSpy;
    var provider;
    var fakeTime;
    var intervalSubscriptionRequest;
    var onChangeSubscriptionRequest;
    var mixedSubscriptionRequest;
    var testAttributeName;
    var value;
    var minIntervalMs;
    var maxIntervalMs;
    var maxNrOfTimes;
    var subscriptionLength;
    var testAttribute;
    var providerSettings;
    var log = LoggerFactory.getLogger("joynr.dispatching.subscription.TestSubscriptionUtil");

    function createSubscriptionInformation(proxy, provider, periodMs, subscriptionLength, onChange, minIntervalMs) {
        var qosSettings;
        if (onChange) {
            if (periodMs !== undefined) {
                qosSettings = new OnChangeWithKeepAliveSubscriptionQos({
                    minIntervalMs: minIntervalMs || 0,
                    maxIntervalMs: periodMs,
                    expiryDateMs: Date.now() + subscriptionLength,
                    alertAfterIntervalMs: 0,
                    publicationTtlMs: 1000
                });
            } else {
                qosSettings = new OnChangeSubscriptionQos({
                    minIntervalMs: minIntervalMs || 0,
                    expiryDateMs: Date.now() + subscriptionLength,
                    publicationTtlMs: 1000
                });
            }
        } else {
            qosSettings = new PeriodicSubscriptionQos({
                periodMs: periodMs,
                expiryDateMs: Date.now() + subscriptionLength,
                alertAfterIntervalMs: 0,
                publicationTtlMs: 1000
            });
        }

        return new SubscriptionInformation(
            SubscriptionInformation.SUBSCRIPTION_TYPE_ATTRIBUTE,
            proxy,
            provider,
            new SubscriptionRequest({
                subscriptionId: "subscriptionId" + uuid(),
                subscribedToName: testAttributeName,
                qos: qosSettings
            })
        );
    }

    function buildString(info) {
        return (
            "{" +
            '"subscriptionType":"' +
            info.subscriptionType +
            '","proxyParticipantId":"' +
            info.proxyParticipantId +
            '","providerParticipantId":"' +
            info.providerParticipantId +
            '","subscriptionId":"' +
            info.subscriptionId +
            '","subscribedToName":"' +
            info.subscribedToName +
            '","qos":{"_typeName":"joynr.OnChangeWithKeepAliveSubscriptionQos","alertAfterIntervalMs":0,"maxIntervalMs":' +
            info.qos.maxIntervalMs +
            ',"minIntervalMs":' +
            info.qos.minIntervalMs +
            ',"expiryDateMs":' +
            info.qos.expiryDateMs +
            ',"publicationTtlMs":1000},"lastPublication":0,"_typeName":"joynr.SubscriptionInformation"}'
        );
    }

    /**
     * Called before each test.
     */
    beforeEach(function() {
        proxyId = "proxy" + uuid();
        providerId = "provider" + uuid();
        testAttributeName = "testAttribute";
    });

    it("serialize single subscription shall work", function() {
        var info = createSubscriptionInformation(proxyId, providerId, 200, 1000, true, 50);

        var subscriptions = {};
        subscriptions[info.subscriptionId] = info;

        var serializedSubscriptions = SubscriptionUtil.serializeSubscriptions(subscriptions);
        var expectedString = "[" + buildString(info) + "]";

        expect(serializedSubscriptions).toBe(expectedString);
    });

    it("serialize multiple subscription shall work", function() {
        var info1 = createSubscriptionInformation(proxyId, providerId, 200, 1000, true, 50);
        var info2 = createSubscriptionInformation(proxyId, providerId, 300, 1000, true, 60);
        var info3 = createSubscriptionInformation(proxyId, providerId, 400, 1000, true, 70);

        var subscriptions = {};
        subscriptions[info1.subscriptionId] = info1;
        subscriptions[info2.subscriptionId] = info2;
        subscriptions[info3.subscriptionId] = info3;

        var serializedSubscriptions = SubscriptionUtil.serializeSubscriptions(subscriptions);
        var expectedString = "[" + buildString(info1) + "," + buildString(info2) + "," + buildString(info3) + "]";

        expect(serializedSubscriptions).toBe(expectedString);
    });

    it("deserialize single subscription shall work", function() {
        var info = createSubscriptionInformation(proxyId, providerId, 200, 1000, true, 50);
        var serializedSubscription = "[" + buildString(info) + "]";

        var subscriptions = SubscriptionUtil.deserializeSubscriptions(serializedSubscription);

        expect(subscriptions[info.subscriptionId].subscriptionId).toBe(info.subscriptionId);
        expect(subscriptions[info.subscriptionId].subscribedToName).toBe(info.subscribedToName);
        expect(subscriptions[info.subscriptionId].proxyParticipantId).toBe(info.proxyParticipantId);
        expect(subscriptions[info.subscriptionId].providerParticipantId).toBe(info.providerParticipantId);
        expect(subscriptions[info.subscriptionId].qos.minIntervalMs).toBe(info.qos.minIntervalMs);
        expect(subscriptions[info.subscriptionId].qos.maxIntervalMs).toBe(info.qos.maxIntervalMs);
    });

    it("createMulticastId", function() {
        expect(SubscriptionUtil.createMulticastId("a", "b")).toEqual("a/b");
        expect(SubscriptionUtil.createMulticastId("a", "b", [])).toEqual("a/b");
        expect(SubscriptionUtil.createMulticastId("a", "b", ["c"])).toEqual("a/b/c");
        expect(SubscriptionUtil.createMulticastId("a", "b", ["c", "d"])).toEqual("a/b/c/d");
        expect(SubscriptionUtil.createMulticastId("a", "b", ["c", "d", "e"])).toEqual("a/b/c/d/e");
        expect(SubscriptionUtil.createMulticastId("a", "b", ["c", "d", "e", "f"])).toEqual("a/b/c/d/e/f");
    });

    it("deserialize multiple subscriptions shall work", function() {
        var info1 = createSubscriptionInformation(proxyId, providerId, 200, 1000, true, 50);
        var info2 = createSubscriptionInformation(proxyId, providerId, 201, 1000, true, 77);
        var serializedSubscription = "[" + buildString(info1) + "," + buildString(info2) + "]";

        var subscriptions = SubscriptionUtil.deserializeSubscriptions(serializedSubscription);

        expect(subscriptions[info1.subscriptionId].subscriptionId).toBe(info1.subscriptionId);
        expect(subscriptions[info1.subscriptionId].subscribedToName).toBe(info1.subscribedToName);
        expect(subscriptions[info1.subscriptionId].proxyParticipantId).toBe(info1.proxyParticipantId);
        expect(subscriptions[info1.subscriptionId].providerParticipantId).toBe(info1.providerParticipantId);
        expect(subscriptions[info1.subscriptionId].qos.minIntervalMs).toBe(info1.qos.minIntervalMs);
        expect(subscriptions[info1.subscriptionId].qos.maxIntervalMs).toBe(info1.qos.maxIntervalMs);

        expect(subscriptions[info2.subscriptionId].subscriptionId).toBe(info2.subscriptionId);
        expect(subscriptions[info2.subscriptionId].subscribedToName).toBe(info2.subscribedToName);
        expect(subscriptions[info2.subscriptionId].proxyParticipantId).toBe(info2.proxyParticipantId);
        expect(subscriptions[info2.subscriptionId].providerParticipantId).toBe(info2.providerParticipantId);
        expect(subscriptions[info2.subscriptionId].qos.minIntervalMs).toBe(info2.qos.minIntervalMs);
        expect(subscriptions[info2.subscriptionId].qos.maxIntervalMs).toBe(info2.qos.maxIntervalMs);

        var subscriptionId;
        for (subscriptionId in subscriptions) {
            if (subscriptions.hasOwnProperty(subscriptionId)) {
                expect(subscriptionId === info1.subscriptionId || subscriptionId === info2.subscriptionId).toBe(true);
            }
        }
    });

    it("deserialize and deserialize single subscription shall work", function() {
        var origin = createSubscriptionInformation(proxyId, providerId, 200, 1000, true, 50);

        var subscriptions = {};
        subscriptions[origin.subscriptionId] = origin;

        var serializedSubscription = SubscriptionUtil.serializeSubscriptions(subscriptions);

        var newSubscriptions = SubscriptionUtil.deserializeSubscriptions(serializedSubscription);

        expect(newSubscriptions[origin.subscriptionId].subscriptionId).toBe(origin.subscriptionId);
        expect(newSubscriptions[origin.subscriptionId].subscribedToName).toBe(origin.subscribedToName);
        expect(newSubscriptions[origin.subscriptionId].proxyParticipantId).toBe(origin.proxyParticipantId);
        expect(newSubscriptions[origin.subscriptionId].providerParticipantId).toBe(origin.providerParticipantId);
        expect(newSubscriptions[origin.subscriptionId].qos.minIntervalMs).toBe(origin.qos.minIntervalMs);
        expect(newSubscriptions[origin.subscriptionId].qos.maxIntervalMs).toBe(origin.qos.maxIntervalMs);
    });

    it("deserialize and deserialize multiple subscriptions shall work", function() {
        var origin1 = createSubscriptionInformation(proxyId, providerId, 200, 1000, true, 50);
        var origin2 = createSubscriptionInformation(proxyId, providerId, 180, 660, true, 70);

        var subscriptions = {};
        subscriptions[origin1.subscriptionId] = origin1;
        subscriptions[origin2.subscriptionId] = origin2;

        var serializedSubscription = SubscriptionUtil.serializeSubscriptions(subscriptions);

        var newSubscriptions = SubscriptionUtil.deserializeSubscriptions(serializedSubscription);

        expect(newSubscriptions[origin1.subscriptionId].subscriptionId).toBe(origin1.subscriptionId);
        expect(newSubscriptions[origin1.subscriptionId].subscribedToName).toBe(origin1.subscribedToName);
        expect(newSubscriptions[origin1.subscriptionId].proxyParticipantId).toBe(origin1.proxyParticipantId);
        expect(newSubscriptions[origin1.subscriptionId].providerParticipantId).toBe(origin1.providerParticipantId);
        expect(newSubscriptions[origin1.subscriptionId].qos.minIntervalMs).toBe(origin1.qos.minIntervalMs);
        expect(newSubscriptions[origin1.subscriptionId].qos.maxIntervalMs).toBe(origin1.qos.maxIntervalMs);

        expect(newSubscriptions[origin2.subscriptionId].subscriptionId).toBe(origin2.subscriptionId);
        expect(newSubscriptions[origin2.subscriptionId].subscribedToName).toBe(origin2.subscribedToName);
        expect(newSubscriptions[origin2.subscriptionId].proxyParticipantId).toBe(origin2.proxyParticipantId);
        expect(newSubscriptions[origin2.subscriptionId].providerParticipantId).toBe(origin2.providerParticipantId);
        expect(newSubscriptions[origin2.subscriptionId].qos.minIntervalMs).toBe(origin2.qos.minIntervalMs);
        expect(newSubscriptions[origin2.subscriptionId].qos.maxIntervalMs).toBe(origin2.qos.maxIntervalMs);
    });
    describe("validatePartitions", function() {
        it("does not throw with valid partitions", function() {
            expect(function() {
                SubscriptionUtil.validatePartitions(["a", "b", "0", "D", "Z"]);
            }).not.toThrow();
        });
        it("throws with invalid partitions", function() {
            expect(function() {
                SubscriptionUtil.validatePartitions([""]);
            }).toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["_"]);
            }).toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["./$"]);
            }).toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["\uD83D"]);
            }).toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["\uDE33"]);
            }).toThrow();
        });
        it("supports the plus sign", function() {
            expect(function() {
                SubscriptionUtil.validatePartitions(["a", "b", "+", "c"]);
            }).not.toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["+"]);
            }).not.toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["a", "+"]);
            }).not.toThrow();
        });
        it("throws if plus sign is conjugated with characters in one partition", function() {
            expect(function() {
                SubscriptionUtil.validatePartitions(["+xy"]);
            }).toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["x+y"]);
            }).toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["xy+"]);
            }).toThrow();
        });
        it("throws if star is not the last partition", function() {
            expect(function() {
                SubscriptionUtil.validatePartitions(["*", "1"]);
            }).toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["1", "*", "2"]);
            }).toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["1", "2", "3", "*", "4", "5", "6"]);
            }).toThrow();
        });
        it("throws if star is conjugated with characters in one partition", function() {
            expect(function() {
                SubscriptionUtil.validatePartitions(["*xy"]);
            }).toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["x*y"]);
            }).toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["xy*"]);
            }).toThrow();
        });
        it("supports combinations of plus sign and star", function() {
            expect(function() {
                SubscriptionUtil.validatePartitions(["+", "+", "*"]);
            }).not.toThrow();
        });
        it("does not throw if star is the last partition", function() {
            expect(function() {
                SubscriptionUtil.validatePartitions(["*"]);
            }).not.toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["1", "*"]);
            }).not.toThrow();
            expect(function() {
                SubscriptionUtil.validatePartitions(["1", "2", "3", "4", "5", "6", "*"]);
            }).not.toThrow();
        });
    });
});
