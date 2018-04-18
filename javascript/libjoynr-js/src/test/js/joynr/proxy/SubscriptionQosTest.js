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
const SubscriptionQos = require("../../../../main/js/joynr/proxy/SubscriptionQos");
const PeriodicSubscriptionQos = require("../../../../main/js/joynr/proxy/PeriodicSubscriptionQos");
const OnChangeSubscriptionQos = require("../../../../main/js/joynr/proxy/OnChangeSubscriptionQos");
const OnChangeWithKeepAliveSubscriptionQos = require("../../../../main/js/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos");
const Date = require("../../../../test/js/global/Date");
describe("libjoynr-js.joynr.proxy.SubscriptionQos", () => {
    const qosSettings = {
        minIntervalMs: 50,
        maxIntervalMs: 51,
        expiryDateMs: 4,
        alertAfterIntervalMs: 80,
        publicationTtlMs: 100
    };

    it("is instantiable", done => {
        expect(new OnChangeWithKeepAliveSubscriptionQos(qosSettings)).toBeDefined();
        done();
    });

    it("is of correct type", done => {
        const subscriptionQos = new OnChangeWithKeepAliveSubscriptionQos(qosSettings);
        expect(subscriptionQos).toBeDefined();
        expect(subscriptionQos).not.toBeNull();
        expect(typeof subscriptionQos === "object").toBeTruthy();
        expect(subscriptionQos instanceof OnChangeWithKeepAliveSubscriptionQos).toEqual(true);
        done();
    });

    function createSubscriptionQos(
        minIntervalMs,
        periodMs,
        onChange,
        expiryDateMs,
        alertAfterIntervalMs,
        publicationTtlMs
    ) {
        let returnValue;
        if (onChange) {
            returnValue = new OnChangeWithKeepAliveSubscriptionQos({
                minIntervalMs,
                maxIntervalMs: periodMs,
                expiryDateMs,
                alertAfterIntervalMs,
                publicationTtlMs
            });
        } else {
            returnValue = new PeriodicSubscriptionQos({
                periodMs,
                expiryDateMs,
                alertAfterIntervalMs,
                publicationTtlMs
            });
        }
        return returnValue;
    }

    function testValues(minIntervalMs, periodMs, onChange, expiryDateMs, alertAfterIntervalMs, publicationTtlMs) {
        const subscriptionQos = createSubscriptionQos(
            minIntervalMs,
            periodMs,
            onChange,
            expiryDateMs,
            alertAfterIntervalMs,
            publicationTtlMs
        );
        let expectedMaxIntervalMs = periodMs;
        if (minIntervalMs < OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS) {
            minIntervalMs = OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS;
        }
        if (minIntervalMs > OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS) {
            minIntervalMs = OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS;
        }

        if (expectedMaxIntervalMs < OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS) {
            expectedMaxIntervalMs = OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS;
        }
        if (expectedMaxIntervalMs > OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS) {
            expectedMaxIntervalMs = OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS;
        }
        if (expectedMaxIntervalMs < minIntervalMs) {
            expectedMaxIntervalMs = minIntervalMs;
        }
        if (onChange) {
            const expectedMinIntervalMs = minIntervalMs;

            expect(subscriptionQos.minIntervalMs).toBe(expectedMinIntervalMs);

            expect(subscriptionQos.maxIntervalMs).toBe(expectedMaxIntervalMs);
        } else {
            expect(subscriptionQos.periodMs).toBe(expectedMaxIntervalMs);
        }
        let expectedPublicationTtlMs = publicationTtlMs;
        if (expectedPublicationTtlMs < SubscriptionQos.MIN_PUBLICATION_TTL_MS) {
            expectedPublicationTtlMs = SubscriptionQos.MIN_PUBLICATION_TTL_MS;
        }
        if (expectedPublicationTtlMs > SubscriptionQos.MAX_PUBLICATION_TTL_MS) {
            expectedPublicationTtlMs = SubscriptionQos.MAX_PUBLICATION_TTL_MS;
        }
        expect(subscriptionQos.publicationTtlMs).toBe(expectedPublicationTtlMs);

        if (expiryDateMs < SubscriptionQos.MIN_EXPIRY_MS) {
            expiryDateMs = SubscriptionQos.MIN_EXPIRY_MS;
        }
        expect(subscriptionQos.expiryDateMs).toBe(expiryDateMs);

        let expectedAlertAfterIntervalMs = alertAfterIntervalMs;
        if (expectedAlertAfterIntervalMs > OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS) {
            expectedAlertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS;
        }
        if (
            expectedAlertAfterIntervalMs !== OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL &&
            expectedAlertAfterIntervalMs < expectedMaxIntervalMs
        ) {
            expectedAlertAfterIntervalMs = expectedMaxIntervalMs;
        }
        expect(subscriptionQos.alertAfterIntervalMs).toBe(expectedAlertAfterIntervalMs);
        return subscriptionQos;
    }

    it("constructs with correct member values", done => {
        //wrong publicationTtlMs
        expect(() => {
            createSubscriptionQos(1, 2, false, 4, 5, -6);
        }).toThrow();
        //wrong periodMs
        expect(() => {
            createSubscriptionQos(1, 2, false, 4, 5, 100);
        }).toThrow();
        //wrong periodMs (exceeds MIN_PERIOD_MS)
        expect(() => {
            createSubscriptionQos(1, PeriodicSubscriptionQos.MIN_PERIOD_MS - 1, false, 4, 5, 100);
        }).toThrow();
        //wrong periodMs (exceeds MAX_PERIOD_MS)
        expect(() => {
            createSubscriptionQos(1, PeriodicSubscriptionQos.MAX_PERIOD_MS + 1, false, 4, 5, 100);
        }).toThrow();
        //wrong alertAfterIntervalMs (shall be higher then the periodMs)
        expect(createSubscriptionQos(1, 50, false, 4, 5, 100).alertAfterIntervalMs).toEqual(50);
        //wrong alertAfterIntervalMs (exceed MAX_ALERT_AFTER_INTERVAL_MS)
        expect(
            createSubscriptionQos(
                1,
                50,
                false,
                4,
                OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS + 1,
                100
            ).alertAfterIntervalMs
        ).toEqual(OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS);
        testValues(1, 50, false, 4, 51, 100);

        //wrong publicationTtlMs
        expect(testValues(-1, -2, true, -4, -5, -6).publicationTtlMs).toEqual(SubscriptionQos.MIN_PUBLICATION_TTL_MS);
        //wrong publicationTtlMs
        expect(testValues(60, 62, true, 10, 100, SubscriptionQos.MAX_PUBLICATION_TTL_MS + 1).publicationTtlMs).toEqual(
            SubscriptionQos.MAX_PUBLICATION_TTL_MS
        );
        //wrong minIntervalMs
        expect(testValues(-1, -2, true, -4, -5, 200).minIntervalMs).toEqual(
            OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS
        );
        //wrong minIntervalMs (exceeds MAX_MIN_INTERVAL_MS)
        expect(
            testValues(OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS + 1, 62, true, 10, 100, 200).minIntervalMs
        ).toEqual(OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS);

        //wrong maxIntervalMs (shall be higher than minIntervalMs)
        expect(testValues(60, -2, true, -4, -5, 200).maxIntervalMs).toEqual(60);
        //wrong maxIntervalMs (below OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS)
        expect(
            testValues(10, OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS - 1, true, -4, -5, 200)
                .maxIntervalMs
        ).toEqual(OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS);
        //wrong maxIntervalMs (exceeds OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS)
        expect(
            testValues(10, OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS + 1, true, -4, -5, 200)
                .maxIntervalMs
        ).toEqual(OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS);
        //wrong alertAfterIntervalMs (shall be higher than maxIntervalMs)
        expect(testValues(60, 62, true, -4, -5, 200).alertAfterIntervalMs).toEqual(62);
        //wrong alertAfterIntervalMs (exceeds MAX_ALERT_AFTER_INTERVAL_MS)
        expect(
            testValues(60, 62, true, -4, PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS + 1, 200)
                .alertAfterIntervalMs
        ).toEqual(PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS);
        //wrong expiryDateMs
        expect(testValues(60, -2, true, -4, 100, 200).expiryDateMs).toEqual(SubscriptionQos.MIN_EXPIRY_MS);
        testValues(60, 62, true, 10, 100, 200);

        //wrong publicationTtlMs
        expect(() => {
            testValues(0, 0, false, 0, 0, 0);
        }).toThrow();
        //wrong periodMs
        expect(() => {
            testValues(0, 0, false, 0, 0, 100);
        }).toThrow();
        testValues(0, 50, false, 0, 0, 100);
        done();
    });

    it("constructs OnChangeWithKeepAliveSubscriptionQos with correct default values", done => {
        const fixture = new OnChangeWithKeepAliveSubscriptionQos();
        expect(fixture.minIntervalMs).toEqual(OnChangeSubscriptionQos.DEFAULT_MIN_INTERVAL_MS);
        expect(fixture.maxIntervalMs).toEqual(OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS);
        expect(fixture.expiryDateMs).toEqual(SubscriptionQos.NO_EXPIRY_DATE);
        expect(fixture.alertAfterIntervalMs).toEqual(
            OnChangeWithKeepAliveSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS
        );
        expect(fixture.publicationTtlMs).toEqual(SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS);
        done();
    });

    it("constructs PeriodicSubscriptionQos with correct default values", done => {
        const fixture = new PeriodicSubscriptionQos();
        expect(fixture.periodMs).toEqual(PeriodicSubscriptionQos.DEFAULT_PERIOD_MS);
        expect(fixture.expiryDateMs).toEqual(SubscriptionQos.NO_EXPIRY_DATE);
        expect(fixture.alertAfterIntervalMs).toEqual(
            OnChangeWithKeepAliveSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS
        );
        expect(fixture.publicationTtlMs).toEqual(SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS);
        done();
    });

    it("SubscriptionQos.clearExpiryDate clears the expiry date", done => {
        const fixture = new OnChangeWithKeepAliveSubscriptionQos({
            expiryDateMs: 1234
        });

        expect(fixture.expiryDateMs).toBe(1234);
        fixture.clearExpiryDate();
        expect(fixture.expiryDateMs).toBe(SubscriptionQos.NO_EXPIRY_DATE);
        done();
    });

    it("PeriodicSubscriptionQos.clearAlertAfterInterval clears the alert after interval", done => {
        const alertAfterIntervalMs = PeriodicSubscriptionQos.DEFAULT_PERIOD_MS + 1;
        const fixture = new PeriodicSubscriptionQos({
            alertAfterIntervalMs
        });

        expect(fixture.alertAfterIntervalMs).toBe(alertAfterIntervalMs);
        fixture.clearAlertAfterInterval();
        expect(fixture.alertAfterIntervalMs).toBe(PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL);
        done();
    });

    it("OnChangeWithKeepAliveSubscriptionQos.clearAlertAfterInterval clears the alert after interval", done => {
        const alertAfterIntervalMs = OnChangeWithKeepAliveSubscriptionQos.DEFAULT_MAX_INTERVAL_MS + 1;
        const fixture = new OnChangeWithKeepAliveSubscriptionQos({
            alertAfterIntervalMs
        });

        expect(fixture.alertAfterIntervalMs).toBe(alertAfterIntervalMs);
        fixture.clearAlertAfterInterval();
        expect(fixture.alertAfterIntervalMs).toBe(OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL);
        done();
    });

    it("subscription qos accepts validity instead of expiry date as constructor member", done => {
        const fakeTime = 374747473;
        const validityMs = 23232;
        spyOn(Date, "now").and.callFake(() => {
            return fakeTime;
        });

        const fixture = new OnChangeWithKeepAliveSubscriptionQos({
            validityMs
        });
        expect(fixture.validityMs).toBe(undefined);
        expect(fixture.expiryDateMs).toBe(fakeTime + validityMs);
        done();
    });

    it("throws on incorrectly typed values", done => {
        // all arguments
        expect(() => {
            createSubscriptionQos(1, 50, false, 4, 80, 100);
        }).not.toThrow();

        // no arguments
        expect(() => {
            createSubscriptionQos(undefined, undefined, undefined, undefined, undefined, undefined);
        }).not.toThrow();

        // arguments 1 wrongly types
        expect(() => {
            createSubscriptionQos({}, 50, true, 4, 80, 100);
        }).toThrow();

        // arguments 2 wrongly types
        expect(() => {
            createSubscriptionQos(1, {}, false, 4, 80, 100);
        }).toThrow();

        // arguments 4 wrongly types
        expect(() => {
            createSubscriptionQos(1, 50, false, {}, 80, 100);
        }).toThrow();

        // arguments 5 wrongly types
        expect(() => {
            createSubscriptionQos(1, 50, false, 4, {}, 100);
        }).toThrow();

        // arguments 6 wrongly types
        expect(() => {
            createSubscriptionQos(1, 50, false, 4, 80, {});
        }).toThrow();
        done();
    });
});
