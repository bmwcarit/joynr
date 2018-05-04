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
const MulticastSubscriptionQos = require("../../../../main/js/joynr/proxy/MulticastSubscriptionQos");
const UtilInternal = require("../../../../main/js/joynr/util/UtilInternal");
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

    function createPeriodicSubscriptionQos(periodMs, expiryDateMs, alertAfterIntervalMs, publicationTtlMs) {
        return new PeriodicSubscriptionQos({
            periodMs,
            expiryDateMs,
            alertAfterIntervalMs,
            publicationTtlMs
        });
    }

    function createOnChangeWithKeepAliveSubscriptionQos(
        minIntervalMs,
        maxIntervalMs,
        expiryDateMs,
        alertAfterIntervalMs,
        publicationTtlMs
    ) {
        return new OnChangeWithKeepAliveSubscriptionQos({
            minIntervalMs,
            maxIntervalMs,
            expiryDateMs,
            alertAfterIntervalMs,
            publicationTtlMs
        });
    }

    function createOnChangeWithKeepAliveOutput(
        minIntervalMs,
        maxIntervalMs,
        expiryDateMs,
        alertAfterIntervalMs,
        publicationTtlMs
    ) {
        return {
            minIntervalMs,
            maxIntervalMs,
            expiryDateMs,
            alertAfterIntervalMs,
            publicationTtlMs
        };
    }

    function createPeriodicOutput(periodMs, expiryDateMs, alertAfterIntervalMs, publicationTtlMs) {
        return {
            periodMs,
            expiryDateMs,
            alertAfterIntervalMs,
            publicationTtlMs
        };
    }

    function compareSubscriptionQos(output, expectedOutput, onChange) {
        expect(output.expiryDateMs).toEqual(expectedOutput.expiryDateMs);
        expect(output.alertAfterIntervalMs).toEqual(expectedOutput.alertAfterIntervalMs);
        expect(output.publicationTtlMs).toEqual(expectedOutput.publicationTtlMs);

        //Do we have an OnChangeWithKeepAliveSubscriptionQos object? => fields min- and maxIntervalMs should exist
        if (onChange) {
            expect(output.minIntervalMs).toEqual(expectedOutput.minIntervalMs);
            expect(output.maxIntervalMs).toEqual(expectedOutput.maxIntervalMs);
            expect(output._typeName).toEqual("joynr.OnChangeWithKeepAliveSubscriptionQos");
        } else {
            //PeriodicSubscriptionQos? => field periodMs should exist
            expect(output.periodMs).toEqual(expectedOutput.periodMs);
            expect(output._typeName).toEqual("joynr.PeriodicSubscriptionQos");
        }
    }

    it("constructs PeriodicSubscriptionQos (including SubscriptionQos) with correct member values", done => {
        //all values regular (lower limits)
        {
            const output = createPeriodicSubscriptionQos(
                PeriodicSubscriptionQos.MIN_PERIOD_MS,
                SubscriptionQos.MIN_EXPIRY_MS,
                PeriodicSubscriptionQos.MIN_PERIOD_MS,
                SubscriptionQos.MIN_PUBLICATION_TTL_MS
            );
            const expectedOutput = createPeriodicOutput(
                PeriodicSubscriptionQos.MIN_PERIOD_MS,
                SubscriptionQos.MIN_EXPIRY_MS,
                PeriodicSubscriptionQos.MIN_PERIOD_MS,
                SubscriptionQos.MIN_PUBLICATION_TTL_MS
            );
            compareSubscriptionQos(output, expectedOutput, false);
        }
        //all values regular (upper limits)
        {
            const output = createPeriodicSubscriptionQos(
                PeriodicSubscriptionQos.MAX_PERIOD_MS,
                UtilInternal.getMaxLongValue(),
                PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS,
                SubscriptionQos.MAX_PUBLICATION_TTL_MS
            );
            const expectedOutput = createPeriodicOutput(
                PeriodicSubscriptionQos.MAX_PERIOD_MS,
                UtilInternal.getMaxLongValue(),
                PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS,
                SubscriptionQos.MAX_PUBLICATION_TTL_MS
            );
            compareSubscriptionQos(output, expectedOutput, false);
        }
        //publicationTtlMs < SubscriptionQos.MIN_PUBLICATION_TTL_MS
        {
            const output = createPeriodicSubscriptionQos(1000, 1000, 1000, SubscriptionQos.MIN_PUBLICATION_TTL_MS - 1);
            const expectedOutput = createPeriodicOutput(1000, 1000, 1000, SubscriptionQos.MIN_PUBLICATION_TTL_MS);
            compareSubscriptionQos(output, expectedOutput, false);
        }
        //publicationTtlMs > SubscriptionQos.MAX_PUBLICATION_TTL_MS
        {
            const output = createPeriodicSubscriptionQos(1000, 1000, 1000, SubscriptionQos.MAX_PUBLICATION_TTL_MS + 1);
            const expectedOutput = createPeriodicOutput(1000, 1000, 1000, SubscriptionQos.MAX_PUBLICATION_TTL_MS);
            compareSubscriptionQos(output, expectedOutput, false);
        }
        //expiryDateMs < SubscriptionQos.MIN_EXPIRY_MS
        {
            const output = createPeriodicSubscriptionQos(1000, SubscriptionQos.MIN_EXPIRY_MS - 1, 1000, 1000);
            const expectedOutput = createPeriodicOutput(1000, SubscriptionQos.MIN_EXPIRY_MS, 1000, 1000);
            compareSubscriptionQos(output, expectedOutput, false);
        }
        //periodMs < PeriodicSubscriptionQos.MIN_PERIOD_MS
        expect(() => {
            createPeriodicSubscriptionQos(PeriodicSubscriptionQos.MIN_PERIOD_MS - 1, 1000, 1000, 1000);
        }).toThrow();

        //periodMs > PeriodicSubscriptionQos.MAX_PERIOD_MS
        expect(() => {
            createPeriodicSubscriptionQos(PeriodicSubscriptionQos.MAX_PERIOD_MS + 1, 1000, 1000, 1000);
        }).toThrow();

        //alertAfterIntervalMs !== PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL && alertAfterIntervalMs < periodMs
        {
            const output = createPeriodicSubscriptionQos(1000, 2000, 999, 3000);
            const expectedOutput = createPeriodicOutput(1000, 2000, 1000, 3000);
            compareSubscriptionQos(output, expectedOutput, false);
        }
        //alertAfterIntervalMs === PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL && alertAfterIntervalMs < periodMs
        {
            const output = createPeriodicSubscriptionQos(
                PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL + 50,
                2000,
                PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL,
                3000
            );
            const expectedOutput = createPeriodicOutput(
                PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL + 50,
                2000,
                PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL,
                3000
            );
            compareSubscriptionQos(output, expectedOutput, false);
        }
        //alertAfterIntervalMs > PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
        {
            const output = createPeriodicSubscriptionQos(
                1000,
                2000,
                PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS + 1,
                3000
            );
            const expectedOutput = createPeriodicOutput(
                1000,
                2000,
                PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS,
                3000
            );
            compareSubscriptionQos(output, expectedOutput, false);
        }
        done();
    });

    it("constructs OnChangeWithKeepAliveSubscriptionQos (including OnChangeSubscriptionQos) with correct member values", done => {
        //all values regular (lower limits)
        {
            const output = createOnChangeWithKeepAliveSubscriptionQos(
                OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS,
                OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS,
                SubscriptionQos.MIN_EXPIRY_MS,
                OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS,
                SubscriptionQos.MIN_PUBLICATION_TTL_MS
            );
            const expectedOutput = createOnChangeWithKeepAliveOutput(
                OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS,
                OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS,
                SubscriptionQos.MIN_EXPIRY_MS,
                OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS,
                SubscriptionQos.MIN_PUBLICATION_TTL_MS
            );
            compareSubscriptionQos(output, expectedOutput, true);
        }
        //all values regular (upper limits)
        {
            const output = createOnChangeWithKeepAliveSubscriptionQos(
                OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS,
                UtilInternal.getMaxLongValue(),
                OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS,
                SubscriptionQos.MAX_PUBLICATION_TTL_MS
            );
            const expectedOutput = createOnChangeWithKeepAliveOutput(
                OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS,
                UtilInternal.getMaxLongValue(),
                OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS,
                SubscriptionQos.MAX_PUBLICATION_TTL_MS
            );
            compareSubscriptionQos(output, expectedOutput, true);
        }
        //minIntervalMs < OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS
        {
            const output = createOnChangeWithKeepAliveSubscriptionQos(
                OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS - 1,
                4321,
                4321,
                4321,
                4321
            );
            const expectedOutput = createOnChangeWithKeepAliveOutput(
                OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS,
                4321,
                4321,
                4321,
                4321
            );
            compareSubscriptionQos(output, expectedOutput, true);
        }
        //minIntervalMs > OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS
        {
            const output = createOnChangeWithKeepAliveSubscriptionQos(
                OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS + 1,
                OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                1234,
                OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                1234
            );
            const expectedOutput = createOnChangeWithKeepAliveOutput(
                OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                1234,
                OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                1234
            );
            compareSubscriptionQos(output, expectedOutput, true);
        }
        //maxIntervalMs < OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS
        {
            const output = createOnChangeWithKeepAliveSubscriptionQos(
                12,
                OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS - 1,
                234,
                345,
                456
            );
            const expectedOutput = createOnChangeWithKeepAliveOutput(
                12,
                OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS,
                234,
                345,
                456
            );
            compareSubscriptionQos(output, expectedOutput, true);
        }
        //maxIntervalMs > OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS
        {
            const output = createOnChangeWithKeepAliveSubscriptionQos(
                1000,
                OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS + 1,
                1000,
                OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS,
                1000
            );
            const expectedOutput = createOnChangeWithKeepAliveOutput(
                1000,
                OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS,
                1000,
                OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS,
                1000
            );
            compareSubscriptionQos(output, expectedOutput, true);
        }
        //maxIntervalMs < minIntervalMs
        {
            const output = createOnChangeWithKeepAliveSubscriptionQos(1000, 100, 2000, 3000, 4000);
            const expectedOutput = createOnChangeWithKeepAliveOutput(1000, 1000, 2000, 3000, 4000);
            compareSubscriptionQos(output, expectedOutput, true);
        }
        //alertAfterIntervalMs !== OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL && alertAfterIntervalMs < maxIntervalMs
        {
            const output = createOnChangeWithKeepAliveSubscriptionQos(1000, 4000, 3000, 2000, 5000);
            const expectedOutput = createOnChangeWithKeepAliveOutput(1000, 4000, 3000, 4000, 5000);
            compareSubscriptionQos(output, expectedOutput, true);
        }
        //alertAfterIntervalMs === OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL && alertAfterIntervalMs < maxIntervalMs
        {
            const output = createOnChangeWithKeepAliveSubscriptionQos(
                PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL + 50,
                4000,
                3000,
                OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL,
                5000
            );
            const expectedOutput = createOnChangeWithKeepAliveOutput(
                PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL + 50,
                4000,
                3000,
                OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL,
                5000
            );
            compareSubscriptionQos(output, expectedOutput, true);
        }
        //alertAfterIntervalMs > OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
        {
            const output = createOnChangeWithKeepAliveSubscriptionQos(
                1000,
                1000,
                1000,
                OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS + 1,
                1000
            );
            const expectedOutput = createOnChangeWithKeepAliveOutput(
                1000,
                1000,
                1000,
                OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS,
                1000
            );
            compareSubscriptionQos(output, expectedOutput, true);
        }
        done();
    });

    it("constructs MulticastSubscriptionQos with correct member values", done => {
        //all values regular (lower limits)
        {
            const output = new MulticastSubscriptionQos({
                expiryDateMs: SubscriptionQos.MIN_EXPIRY_MS,
                publicationTtlMs: SubscriptionQos.MIN_PUBLICATION_TTL_MS
            });
            expect(output.expiryDateMs).toEqual(SubscriptionQos.MIN_EXPIRY_MS);
            expect(output.publicationTtlMs).toEqual(SubscriptionQos.MIN_PUBLICATION_TTL_MS);
            expect(output._typeName).toEqual("joynr.MulticastSubscriptionQos");
        }
        //all values regular (random values)
        {
            const output = new MulticastSubscriptionQos({
                expiryDateMs: 1234,
                publicationTtlMs: 5678
            });
            expect(output.expiryDateMs).toEqual(1234);
            expect(output.publicationTtlMs).toEqual(5678);
            expect(output._typeName).toEqual("joynr.MulticastSubscriptionQos");
        }
        //all values regular (upper limits)
        {
            const output = new MulticastSubscriptionQos({
                expiryDateMs: UtilInternal.getMaxLongValue(),
                publicationTtlMs: SubscriptionQos.MAX_PUBLICATION_TTL_MS
            });
            expect(output.expiryDateMs).toEqual(UtilInternal.getMaxLongValue());
            expect(output.publicationTtlMs).toEqual(SubscriptionQos.MAX_PUBLICATION_TTL_MS);
            expect(output._typeName).toEqual("joynr.MulticastSubscriptionQos");
        }
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

    it("constructs MulticastSubscriptionQos with correct default values", done => {
        const fixture = new MulticastSubscriptionQos();
        expect(fixture.expiryDateMs).toEqual(SubscriptionQos.NO_EXPIRY_DATE);
        expect(fixture.publicationTtlMs).toEqual(SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS);
        done();
    });

    it("throws on incorrectly typed values", done => {
        // all arguments ok in PeriodicSubscriptionQos
        expect(() => {
            createPeriodicSubscriptionQos(50, 4, 80, 100);
        }).not.toThrow();

        // all arguments ok in OnChangeWithKeepAliveSubscriptionQos
        expect(() => {
            createOnChangeWithKeepAliveSubscriptionQos(1, 50, 4, 80, 100);
        }).not.toThrow();

        // no arguments defined in PeriodicSubscriptionQos
        expect(() => {
            createPeriodicSubscriptionQos(undefined, undefined, undefined, undefined);
        }).not.toThrow();

        // no arguments defined in OnChangeWithKeepAliveSubscriptionQos
        expect(() => {
            createOnChangeWithKeepAliveSubscriptionQos(undefined, undefined, undefined, undefined, undefined);
        }).not.toThrow();

        // argument minIntervalMs wrongly typed in OnChangeWithKeepAliveSubscriptionQos
        expect(() => {
            createOnChangeWithKeepAliveSubscriptionQos({}, 50, 4, 80, 100);
        }).toThrow();

        // argument maxIntervalMs wrongly typed in OnChangeWithKeepAliveSubscriptionQos
        expect(() => {
            createOnChangeWithKeepAliveSubscriptionQos(1, {}, 4, 80, 100);
        }).toThrow();

        // argument expiryDateMs wrongly typed in OnChangeWithKeepAliveSubscriptionQos
        expect(() => {
            createOnChangeWithKeepAliveSubscriptionQos(1, 50, {}, 80, 100);
        }).toThrow();

        // argument alertAfterIntervalMs wrongly typed in OnChangeWithKeepAliveSubscriptionQos
        expect(() => {
            createOnChangeWithKeepAliveSubscriptionQos(1, 50, 4, {}, 100);
        }).toThrow();

        // argument publicationTtlMs wrongly typed in OnChangeWithKeepAliveSubscriptionQos
        expect(() => {
            createOnChangeWithKeepAliveSubscriptionQos(1, 50, 4, 80, {});
        }).toThrow();

        // argument periodMs wrongly typed in PeriodicSubscriptionQos
        expect(() => {
            createPeriodicSubscriptionQos({}, 4, 80, 100);
        }).toThrow();

        // argument expiryDateMs wrongly typed in PeriodicSubscriptionQos
        expect(() => {
            createPeriodicSubscriptionQos(50, {}, 80, 100);
        }).toThrow();

        // argument alertAfterIntervalMs wrongly typed in PeriodicSubscriptionQos
        expect(() => {
            createPeriodicSubscriptionQos(50, 4, {}, 100);
        }).toThrow();

        // argument publicationTtlMs wrongly typed in PeriodicSubscriptionQos
        expect(() => {
            createPeriodicSubscriptionQos(50, 4, 80, {});
        }).toThrow();
        done();
    });
});
