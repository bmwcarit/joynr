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
    "joynr/proxy/OnChangeWithKeepAliveSubscriptionQos"
], function(
        SubscriptionQos,
        PeriodicSubscriptionQos,
        OnChangeSubscriptionQos,
        OnChangeWithKeepAliveSubscriptionQos) {

    describe("libjoynr-js.joynr.proxy.SubscriptionQos", function() {

        var qosSettings = {
            minInterval : 50,
            maxInterval : 51,
            expiryDateMs : 4,
            alertAfterInterval : 80,
            publicationTtl : 100
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
                minInterval,
                period,
                onChange,
                expiryDateMs,
                alertAfterInterval,
                publicationTtl) {
            var returnValue;
            if (onChange) {
                returnValue = new OnChangeWithKeepAliveSubscriptionQos({
                    minInterval : minInterval,
                    maxInterval : period,
                    expiryDateMs : expiryDateMs,
                    alertAfterInterval : alertAfterInterval,
                    publicationTtl : publicationTtl
                });
            } else {
                returnValue = new PeriodicSubscriptionQos({
                    period : period,
                    expiryDateMs : expiryDateMs,
                    alertAfterInterval : alertAfterInterval,
                    publicationTtl : publicationTtl
                });
            }
            return returnValue;
        }

        function testValues(
                minInterval,
                period,
                onChange,
                expiryDateMs,
                alertAfterInterval,
                publicationTtl) {
            var subscriptionQos =
                    createSubscriptionQos(
                            minInterval,
                            period,
                            onChange,
                            expiryDateMs,
                            alertAfterInterval,
                            publicationTtl);
            var expectedMaxInterval = period;
            if (onChange) {
                var expectedMinInterval = minInterval;

                expect(subscriptionQos.minInterval).toBe(expectedMinInterval);

                expect(subscriptionQos.maxInterval).toBe(expectedMaxInterval);
            } else {
                expect(subscriptionQos.period).toBe(expectedMaxInterval);
            }
            var expectedPulicationTtl = publicationTtl;
            expect(subscriptionQos.publicationTtl).toBe(expectedPulicationTtl);

            expect(subscriptionQos.expiryDateMs).toBe(expiryDateMs);

            var expectedAlertAfterInterval = alertAfterInterval;
            expect(subscriptionQos.alertAfterInterval).toBe(expectedAlertAfterInterval);
        }

        it("constructs with correct member values", function() {
            //wrong publicationTtl
            expect(function() {
                createSubscriptionQos(1, 2, false, 4, 5, -6);
            }).toThrow();
            //wrong period
            expect(function() {
                createSubscriptionQos(1, 2, false, 4, 5, 100);
            }).toThrow();
            //wrong alertAfterInterval (shall be higher then the period)
            expect(function() {
                createSubscriptionQos(1, 50, false, 4, 5, 100);
            }).toThrow();
            testValues(1, 50, false, 4, 51, 100);

            //wrong publicationTtl
            expect(function() {
                testValues(-1, -2, true, -4, -5, -6);
            }).toThrow();
            //wrong minInterval
            expect(function() {
                testValues(-1, -2, true, -4, -5, 200);
            }).toThrow();
            //wrong maxInterval (shall be higher than minInterval)
            expect(function() {
                testValues(60, -2, true, -4, -5, 200);
            }).toThrow();
            //wrong alertAfterInterval (shall be higher than maxInterval)
            expect(function() {
                testValues(60, 62, true, -4, -5, 200);
            }).toThrow();
            //wrong expiryDate
            expect(function() {
                testValues(60, -2, true, -4, 100, 200);
            }).toThrow();
            testValues(60, 62, true, 10, 100, 200);

            //wrong publicationTtl
            expect(function() {
                testValues(0, 0, false, 0, 0, 0);
            }).toThrow();
            //wrong period
            expect(function() {
                testValues(0, 0, false, 0, 0, 100);
            }).toThrow();
            testValues(0, 50, false, 0, 0, 100);

        });

        it("constructs with correct default values", function() {
            expect(new OnChangeWithKeepAliveSubscriptionQos()).toEqual(
                    new OnChangeWithKeepAliveSubscriptionQos({
                        minInterval : OnChangeSubscriptionQos.MIN_INTERVAL,
                        expiryDateMs : SubscriptionQos.NO_EXPIRY_DATE, // see comment in SubscriptionQos (cause: javascript floating point stuff)
                        alertAfterInterval : OnChangeWithKeepAliveSubscriptionQos.NEVER_ALERT,
                        publicationTtl : SubscriptionQos.DEFAULT_PUBLICATION_TTL
                    }));
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
