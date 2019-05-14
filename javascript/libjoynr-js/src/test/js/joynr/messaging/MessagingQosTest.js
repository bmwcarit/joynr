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
const defaultMessagingSettings = require("../../../../main/js/joynr/start/settings/defaultMessagingSettings");
const MessagingQos = require("../../../../main/js/joynr/messaging/MessagingQos");
const MessagingQosEffort = require("../../../../main/js/joynr/messaging/MessagingQosEffort");

describe("libjoynr-js.joynr.messaging.MessagingQos", () => {
    it("is instantiable", () => {
        expect(new MessagingQos()).toBeDefined();
        expect(
            new MessagingQos({
                ttl: 60000
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                ttl: 60000,
                effort: MessagingQosEffort.BEST_EFFORT
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                ttl: 60000,
                encrypt: true
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                ttl: 60000,
                effort: MessagingQosEffort.BEST_EFFORT,
                encrypt: true
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                effort: MessagingQosEffort.BEST_EFFORT,
                encrypt: true
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                encrypt: true
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                ttl: 60000,
                compress: true
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                ttl: 60000,
                effort: MessagingQosEffort.BEST_EFFORT,
                compress: true
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                effort: MessagingQosEffort.BEST_EFFORT,
                compress: true
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                compress: true
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                ttl: 60000,
                encrypt: true,
                compress: true
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                ttl: 60000,
                effort: MessagingQosEffort.BEST_EFFORT,
                encrypt: true,
                compress: true
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                effort: MessagingQosEffort.BEST_EFFORT,
                encrypt: true,
                compress: true
            })
        ).toBeDefined();
        expect(
            new MessagingQos({
                compress: true,
                encrypt: true
            })
        ).toBeDefined();
    });

    it("is of correct type", () => {
        const emptyMessagingQos = new MessagingQos();
        expect(emptyMessagingQos).toBeDefined();
        expect(emptyMessagingQos).not.toBeNull();
        expect(typeof emptyMessagingQos === "object").toBeTruthy();
        expect(emptyMessagingQos instanceof MessagingQos).toBeTruthy();

        const defaultMessagingQos = new MessagingQos();
        expect(defaultMessagingQos).toBeDefined();
        expect(defaultMessagingQos).not.toBeNull();
        expect(typeof defaultMessagingQos === "object").toBeTruthy();
        expect(defaultMessagingQos instanceof MessagingQos).toEqual(true);
    });

    it("constructs correct default object", () => {
        expect(new MessagingQos()).toEqual(
            new MessagingQos({
                ttl: MessagingQos.DEFAULT_TTL
            })
        );
        expect(new MessagingQos().encrypt).toEqual(false);
        expect(new MessagingQos().compress).toEqual(false);
    });

    function testTtlValues(ttl) {
        const messagingQos = new MessagingQos({
            ttl
        });
        expect(messagingQos.ttl).toBe(ttl);
    }

    it("constructs with correct TTL values", () => {
        testTtlValues(123456, 1234567);
        testTtlValues(0, 0);
        testTtlValues(-123456, -1234567);
    });

    it("prevents ttl values larger than maxTtl", () => {
        expect(
            new MessagingQos({
                ttl: defaultMessagingSettings.MAX_MESSAGING_TTL_MS + 1
            })
        ).toEqual(
            new MessagingQos({
                ttl: defaultMessagingSettings.MAX_MESSAGING_TTL_MS
            })
        );
    });

    function testEffortValues(effort, expected) {
        const messagingQos = new MessagingQos({
            effort
        });
        expect(messagingQos.effort).toBe(expected);
    }

    it("constructs with correct effort values", () => {
        testEffortValues(MessagingQosEffort.NORMAL, MessagingQosEffort.NORMAL);
        testEffortValues(MessagingQosEffort.BEST_EFFORT, MessagingQosEffort.BEST_EFFORT);
        testEffortValues(null, MessagingQosEffort.NORMAL);
        testEffortValues("not an enum value", MessagingQosEffort.NORMAL);
    });

    function testEncryptValues(encrypt) {
        const messagingQos = new MessagingQos({
            encrypt
        });
        expect(messagingQos.encrypt).toBe(encrypt);
    }

    it("constructs with correct encrypt values", () => {
        testEncryptValues(false);
        testEncryptValues(true);
    });

    function testCompressValues(compress) {
        const messagingQos = new MessagingQos({
            compress
        });
        expect(messagingQos.compress).toBe(compress);
    }

    it("constructs with correct compress values", () => {
        testCompressValues(false);
        testCompressValues(true);
    });

    const runsWithCustomHeaders = [
        {
            params: {
                key: "key",
                value: "value"
            },
            ok: true
        },
        {
            params: {
                key: "1key",
                value: "1value"
            },
            ok: true
        },
        {
            params: {
                key: "key1",
                value: "value1"
            },
            ok: true
        },
        {
            params: {
                key: "key-1",
                value: "value1"
            },
            ok: true
        },
        {
            params: {
                key: "123",
                value: "123"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one;two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one:two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one,two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one+two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one&two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one?two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one-two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one.two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one*two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one/two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "one\\two"
            },
            ok: true
        },
        {
            params: {
                key: "key",
                value: "wrongvalue$"
            },
            ok: false
        },
        {
            params: {
                key: "key",
                value: "wrongvalue%"
            },
            ok: false
        },
        {
            params: {
                key: "wrongkey ",
                value: "value"
            },
            ok: false
        },
        {
            params: {
                key: "wrongkey;",
                value: "value"
            },
            ok: false
        },
        {
            params: {
                key: "wrongkey:",
                value: "value"
            },
            ok: false
        },
        {
            params: {
                key: "wrongkey,",
                value: "value"
            },
            ok: false
        },
        {
            params: {
                key: "wrongkey+",
                value: "value"
            },
            ok: false
        },
        {
            params: {
                key: "wrongkey&",
                value: "value"
            },
            ok: false
        },
        {
            params: {
                key: "wrongkey?",
                value: "value"
            },
            ok: false
        },
        {
            params: {
                key: "wrongkey.",
                value: "value"
            },
            ok: false
        },
        {
            params: {
                key: "wrongkey*",
                value: "value"
            },
            ok: false
        },
        {
            params: {
                key: "wrongkey/",
                value: "value"
            },
            ok: false
        },
        {
            params: {
                key: "wrongkey\\",
                value: "value"
            },
            ok: false
        }
    ];
    runsWithCustomHeaders.forEach(run => {
        const expectedTo = run.ok ? "passes" : "fails";
        const params = run.params;
        it(`setting custom header ${expectedTo} when key: ${params.key} value: ${params.value}`, () => {
            const key = params.key;
            const value = params.value;
            const messagingQos = new MessagingQos();
            if (run.ok) {
                messagingQos.putCustomMessageHeader(key, value);
                expect(messagingQos.customHeaders[key]).toEqual(value);
            } else {
                expect(() => {
                    messagingQos.putCustomMessageHeader(key, value);
                }).toThrow();
            }
        });
    });
});
