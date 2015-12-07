/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

define([
    "joynr/start/settings/defaultMessagingSettings",
    "joynr/messaging/MessagingQos"
], function(defaultMessagingSettings, MessagingQos) {

    describe("libjoynr-js.joynr.messaging.MessagingQos", function() {
        it("is instantiable", function() {
            expect(new MessagingQos()).toBeDefined();
            expect(new MessagingQos({
                ttl : 60000
            })).toBeDefined();
        });

        it("is of correct type", function() {
            var emptyMessagingQos = new MessagingQos();
            expect(emptyMessagingQos).toBeDefined();
            expect(emptyMessagingQos).not.toBeNull();
            expect(typeof emptyMessagingQos === "object").toBeTruthy();
            expect(emptyMessagingQos instanceof MessagingQos).toBeTruthy();

            var defaultMessagingQos = new MessagingQos();
            expect(defaultMessagingQos).toBeDefined();
            expect(defaultMessagingQos).not.toBeNull();
            expect(typeof defaultMessagingQos === "object").toBeTruthy();
            expect(defaultMessagingQos instanceof MessagingQos).toEqual(true);
        });

        it("constructs correct default object", function() {
            expect(new MessagingQos()).toEqual(new MessagingQos({
                ttl : MessagingQos.DEFAULT_TTL
            }));
        });

        function testValues(ttl) {
            var messagingQos = new MessagingQos({
                ttl : ttl
            });
            expect(messagingQos.ttl).toBe(ttl);
        }

        it("constructs with correct member values", function() {
            testValues(123456, 1234567);
            testValues(0, 0);
            testValues(-123456, -1234567);
        });

        it("prevents ttl values larger than maxTtl", function() {
            expect(new MessagingQos({
                ttl : defaultMessagingSettings.MAX_MESSAGING_TTL_MS + 1
            })).toEqual(new MessagingQos({
                ttl : defaultMessagingSettings.MAX_MESSAGING_TTL_MS
            }));
        });

        var runsWithCustomHeaders = [
            {
                params : {
                    key : "key",
                    value : "value"
                },
                ok : true
            },
            {
                params : {
                    key : "1key",
                    value : "1value"
                },
                ok : true
            },
            {
                params : {
                    key : "key1",
                    value : "value1"
                },
                ok : true
            },
            {
                params : {
                    key : "key-1",
                    value : "value1"
                },
                ok : true
            },
            {
                params : {
                    key : "123",
                    value : "123"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one;two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one:two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one,two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one+two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one&two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one?two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one-two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one.two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one*two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one/two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "one\\two"
                },
                ok : true
            },
            {
                params : {
                    key : "key",
                    value : "wrongvalue$"
                },
                ok : false
            },
            {
                params : {
                    key : "key",
                    value : "wrongvalue%"
                },
                ok : false
            },
            {
                params : {
                    key : "wrongkey ",
                    value : "value"
                },
                ok : false
            },
            {
                params : {
                    key : "wrongkey;",
                    value : "value"
                },
                ok : false
            },
            {
                params : {
                    key : "wrongkey:",
                    value : "value"
                },
                ok : false
            },
            {
                params : {
                    key : "wrongkey,",
                    value : "value"
                },
                ok : false
            },
            {
                params : {
                    key : "wrongkey+",
                    value : "value"
                },
                ok : false
            },
            {
                params : {
                    key : "wrongkey&",
                    value : "value"
                },
                ok : false
            },
            {
                params : {
                    key : "wrongkey?",
                    value : "value"
                },
                ok : false
            },
            {
                params : {
                    key : "wrongkey.",
                    value : "value"
                },
                ok : false
            },
            {
                params : {
                    key : "wrongkey*",
                    value : "value"
                },
                ok : false
            },
            {
                params : {
                    key : "wrongkey/",
                    value : "value"
                },
                ok : false
            },
            {
                params : {
                    key : "wrongkey\\",
                    value : "value"
                },
                ok : false
            }
        ];
        runsWithCustomHeaders.forEach(function(run) {
            var expectedTo = run.ok ? "passes" : "fails";
            var params = run.params;
            it("setting custom header "
                + expectedTo
                + " when key: "
                + params.key
                + " value: "
                + params.value, function() {
                var key = params.key;
                var value = params.value;
                var messagingQos = new MessagingQos();
                if (run.ok) {
                    messagingQos.putCustomMessageHeader(key, value);
                    expect(messagingQos.customHeaders[key]).toEqual(value);
                } else {
                    expect(function() {
                        messagingQos.putCustomMessageHeader(key, value);
                    }).toThrow();
                }
            });
        });
    });

}); // require
