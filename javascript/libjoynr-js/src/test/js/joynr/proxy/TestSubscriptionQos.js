/*global joynrTestRequire: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

joynrTestRequire("joynr/proxy/TestSubscriptionQos", [
    "joynr/proxy/SubscriptionQos",
    "joynr/proxy/PeriodicSubscriptionQos",
    "joynr/proxy/OnChangeSubscriptionQos",
    "joynr/proxy/OnChangeWithKeepAliveSubscriptionQos",
    "Date"
], function(
        SubscriptionQos,
        PeriodicSubscriptionQos,
        OnChangeSubscriptionQos,
        OnChangeWithKeepAliveSubscriptionQos,
        Date) {

    describe("libjoynr-js.joynr.proxy.SubscriptionQos", function() {

        var qosSettings = {
            minIntervalMs : 50,
            maxIntervalMs : 51,
            expiryDateMs : 4,
            alertAfterIntervalMs : 80,
            publicationTtlMs : 100
        };

        it("is instantiable", function() {
            expect(new OnChangeWithKeepAliveSubscriptionQos(qosSettings)).toBeDefined();
        });

        it("is of correct type", function() {
            var subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(qosSettings);
            expect(subscriptionQos).toBeDefined();
            expect(subscriptionQos).not.toBeNull();
            expect(typeof subscriptionQos === "object").toBeTruthy();
            expect(subscriptionQos instanceof OnChangeWithKeepAliveSubscriptionQos).toEqual(true);
        });

        function createSubscriptionQos(
                minIntervalMs,
                periodMs,
                onChange,
                expiryDateMs,
                alertAfterIntervalMs,
                publicationTtlMs) {
            var returnValue;
            if (onChange) {
                returnValue = new OnChangeWithKeepAliveSubscriptionQos({
                    minIntervalMs : minIntervalMs,
                    maxIntervalMs : periodMs,
                    expiryDateMs : expiryDateMs,
                    alertAfterIntervalMs : alertAfterIntervalMs,
                    publicationTtlMs : publicationTtlMs
                });
            } else {
                returnValue = new PeriodicSubscriptionQos({
                    periodMs : periodMs,
                    expiryDateMs : expiryDateMs,
                    alertAfterIntervalMs : alertAfterIntervalMs,
                    publicationTtlMs : publicationTtlMs
                });
            }
            return returnValue;
        }

        function testValues(
                minIntervalMs,
                periodMs,
                onChange,
                expiryDateMs,
                alertAfterIntervalMs,
                publicationTtlMs) {
            var subscriptionQos =
                    createSubscriptionQos(
                            minIntervalMs,
                            periodMs,
                            onChange,
                            expiryDateMs,
                            alertAfterIntervalMs,
                            publicationTtlMs);
            var expectedMaxIntervalMs = periodMs;
            if (onChange) {
                var expectedMinIntervalMs = minIntervalMs;

                expect(subscriptionQos.minIntervalMs).toBe(expectedMinIntervalMs);

                expect(subscriptionQos.maxIntervalMs).toBe(expectedMaxIntervalMs);
            } else {
                expect(subscriptionQos.periodMs).toBe(expectedMaxIntervalMs);
            }
            var expectedPublicationTtlMs = publicationTtlMs;
            expect(subscriptionQos.publicationTtlMs).toBe(expectedPublicationTtlMs);

            expect(subscriptionQos.expiryDateMs).toBe(expiryDateMs);

            var expectedAlertAfterIntervalMs = alertAfterIntervalMs;
            expect(subscriptionQos.alertAfterIntervalMs).toBe(expectedAlertAfterIntervalMs);
        }

        it("constructs with correct member values", function() {
            //wrong publicationTtlMs
            expect(function() {
                createSubscriptionQos(1, 2, false, 4, 5, -6);
            }).toThrow();
            //wrong periodMs
            expect(function() {
                createSubscriptionQos(1, 2, false, 4, 5, 100);
            }).toThrow();
            //wrong periodMs (exceeds MIN_PERIOD_MS)
            expect(
                    function() {
                        createSubscriptionQos(
                                1,
                                PeriodicSubscriptionQos.MIN_PERIOD_MS - 1,
                                false,
                                4,
                                5,
                                100);
                    }).toThrow();
            //wrong periodMs (exceeds MAX_PERIOD_MS)
            expect(
                    function() {
                        createSubscriptionQos(
                                1,
                                PeriodicSubscriptionQos.MAX_PERIOD_MS + 1,
                                false,
                                4,
                                5,
                                100);
                    }).toThrow();
            //wrong alertAfterIntervalMs (shall be higher then the periodMs)
            expect(function() {
                createSubscriptionQos(1, 50, false, 4, 5, 100);
            }).toThrow();
            testValues(1, 50, false, 4, 51, 100);

            //wrong publicationTtlMs
            expect(function() {
                testValues(-1, -2, true, -4, -5, -6);
            }).toThrow();
            //wrong publicationTtlMs
            expect(function() {
                testValues(60, 62, true, 10, 100, SubscriptionQos.MAX_PUBLICATION_TTL_MS + 1);
            }).toThrow();
            //wrong minIntervalMs
            expect(function() {
                testValues(-1, -2, true, -4, -5, 200);
            }).toThrow();
            //wrong minIntervalMs (exceeds MAX_MIN_INTERVAL_MS)
            expect(
                    function() {
                        testValues(
                                OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS + 1,
                                62,
                                true,
                                10,
                                100,
                                200);
                    }).toThrow();

            //wrong maxIntervalMs (shall be higher than minIntervalMs)
            expect(function() {
                testValues(60, -2, true, -4, -5, 200);
            }).toThrow();
            //wrong alertAfterIntervalMs (shall be higher than maxIntervalMs)
            expect(function() {
                testValues(60, 62, true, -4, -5, 200);
            }).toThrow();
            //wrong alertAfterIntervalMs (exceeds MAX_ALERT_AFTER_INTERVAL_MS)
            expect(
                    function() {
                        testValues(
                                60,
                                62,
                                true,
                                -4,
                                PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS + 1,
                                200);
                    }).toThrow();
            //wrong expiryDate
            expect(function() {
                testValues(60, -2, true, -4, 100, 200);
            }).toThrow();
            testValues(60, 62, true, 10, 100, 200);

            //wrong publicationTtlMs
            expect(function() {
                testValues(0, 0, false, 0, 0, 0);
            }).toThrow();
            //wrong periodMs
            expect(function() {
                testValues(0, 0, false, 0, 0, 100);
            }).toThrow();
            testValues(0, 50, false, 0, 0, 100);

        });

        it(
                "constructs OnChangeWithKeepAliveSubscriptionQos with correct default values",
                function() {
                    var fixture = new OnChangeWithKeepAliveSubscriptionQos();
                    expect(fixture.minIntervalMs).toEqual(
                            OnChangeSubscriptionQos.DEFAULT_MIN_INTERVAL_MS);
                    expect(fixture.expiryDateMs).toEqual(SubscriptionQos.NO_EXPIRY_DATE);
                    expect(fixture.alertAfterIntervalMs).toEqual(
                            OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL);
                    expect(fixture.publicationTtlMs).toEqual(
                            SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS);
                });

        it("constructs PeriodicSubscriptionQos with correct default values", function() {
            var fixture = new PeriodicSubscriptionQos();
            expect(fixture.periodMs).toEqual(PeriodicSubscriptionQos.DEFAULT_PERIOD_MS);
            expect(fixture.expiryDateMs).toEqual(SubscriptionQos.NO_EXPIRY_DATE);
            expect(fixture.alertAfterIntervalMs).toEqual(
                    OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL);
            expect(fixture.publicationTtlMs).toEqual(SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS);
        });

        it("SubscriptionQos.clearExpiryDate clears the expiry date", function() {
            var fixture = new OnChangeWithKeepAliveSubscriptionQos({
                expiryDateMs : 1234
            });

            expect(fixture.expiryDateMs).toBe(1234);
            fixture.clearExpiryDate();
            expect(fixture.expiryDateMs).toBe(SubscriptionQos.NO_EXPIRY_DATE);
        });

        it("create deprecated subscriptionQos objects", function() {
            var deprecatedQos = new OnChangeWithKeepAliveSubscriptionQos({
                minInterval : 0,
                maxInterval : 50,
                expiryDate : 1000,
                alertAfterInterval : 200,
                publicationTtl : 100

            });
            expect(deprecatedQos.expiryDateMs).toEqual(1000);
            expect(deprecatedQos.publicationTtlMs).toEqual(100);
            expect(deprecatedQos.alertAfterIntervalMs).toEqual(200);
            expect(deprecatedQos.minIntervalMs).toEqual(0);
            expect(deprecatedQos.maxIntervalMs).toEqual(50);
        });

        it(
                "subscription qos accepts validity instead of expiry date as constructor member",
                function() {
                    var fakeTime = 374747473;
                    var validityMs = 23232;
                    spyOn(Date, "now").andCallFake(function() {
                        return fakeTime;
                    });

                    var fixture = new OnChangeWithKeepAliveSubscriptionQos({
                        validityMs : validityMs
                    });
                    expect(fixture.validityMs).toBe(undefined);
                    expect(fixture.expiryDateMs).toBe(fakeTime + validityMs);
                });

        it("throws on incorrectly typed values", function() {
            // all arguments
            expect(function() {
                createSubscriptionQos(1, 50, false, 4, 80, 100);
            }).not.toThrow();

            // no arguments
            expect(function() {
                createSubscriptionQos(
                        undefined,
                        undefined,
                        undefined,
                        undefined,
                        undefined,
                        undefined);
            }).not.toThrow();

            // arguments 1 wrongly types
            expect(function() {
                createSubscriptionQos({}, 50, true, 4, 80, 100);
            }).toThrow();

            // arguments 2 wrongly types
            expect(function() {
                createSubscriptionQos(1, {}, false, 4, 80, 100);
            }).toThrow();

            // arguments 4 wrongly types
            expect(function() {
                createSubscriptionQos(1, 50, false, {}, 80, 100);
            }).toThrow();

            // arguments 5 wrongly types
            expect(function() {
                createSubscriptionQos(1, 50, false, 4, {}, 100);
            }).toThrow();

            // arguments 6 wrongly types
            expect(function() {
                createSubscriptionQos(1, 50, false, 4, 80, {});
            }).toThrow();

        });
    });

}); // require
