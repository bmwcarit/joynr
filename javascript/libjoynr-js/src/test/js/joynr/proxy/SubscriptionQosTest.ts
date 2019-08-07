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
import SubscriptionQos from "../../../../main/js/joynr/proxy/SubscriptionQos";
import PeriodicSubscriptionQos from "../../../../main/js/joynr/proxy/PeriodicSubscriptionQos";
import OnChangeSubscriptionQos from "../../../../main/js/joynr/proxy/OnChangeSubscriptionQos";
import OnChangeWithKeepAliveSubscriptionQos from "../../../../main/js/joynr/proxy/OnChangeWithKeepAliveSubscriptionQos";
import MulticastSubscriptionQos from "../../../../main/js/joynr/proxy/MulticastSubscriptionQos";
import * as UtilInternal from "../../../../main/js/joynr/util/UtilInternal";

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

    // compare existing member values of subscriptionQos to the corresponding ones in expectedOutput
    function compareSubscriptionQosSettings(subscriptionQos: any, expectedOutput: any, specification?: any) {
        // fields expiryDateMs and publicationTtlMs should exist in any case
        expect(subscriptionQos.expiryDateMs).toEqual(expectedOutput.expiryDateMs);
        expect(subscriptionQos.publicationTtlMs).toEqual(expectedOutput.publicationTtlMs);
        switch (specification) {
            case "PeriodicSubscriptionQos":
                // Do we have a PeriodicSubscriptionQos object? => fields periodMs and alertAfterIntervalMs should exist
                expect(subscriptionQos.periodMs).toEqual(expectedOutput.periodMs);
                expect(subscriptionQos.alertAfterIntervalMs).toEqual(expectedOutput.periodicAlertAfterIntervalMs);
                break;
            case "OnChangeWithKeepAliveSubscriptionQos":
                // Do we have an OnChangeWithKeepAliveSubscriptionQos object? => fields min- and maxIntervalMs and alertAfterIntervalMs should exist
                expect(subscriptionQos.maxIntervalMs).toEqual(expectedOutput.maxIntervalMs);
                expect(subscriptionQos.alertAfterIntervalMs).toEqual(expectedOutput.onChangeAlertAfterIntervalMs);
            // fall through
            case "OnChangeSubscriptionQos":
                // Do we have an OnChangeSubscriptionQos object? => field minIntervalMs should exist
                expect(subscriptionQos.minIntervalMs).toEqual(expectedOutput.minIntervalMs);
                break;
            default:
                // no further fields to check
                break;
        }
    }

    // for each input: Call constructors of SubscriptionQos and every subclass and check output
    function checkSettingsInSubscriptionQosAndChildren(input: any, expectedOutput: any) {
        // check if SubscriptionQos is constructed with correct member values
        compareSubscriptionQosSettings(
            new SubscriptionQos({
                expiryDateMs: input.expiryDateMs,
                publicationTtlMs: input.publicationTtlMs
            }),
            expectedOutput
        );

        compareSubscriptionQosSettings(new MulticastSubscriptionQos(input), expectedOutput);
        compareSubscriptionQosSettings(
            new PeriodicSubscriptionQos({
                expiryDateMs: input.expiryDateMs,
                alertAfterIntervalMs: input.periodicAlertAfterIntervalMs,
                publicationTtlMs: input.publicationTtlMs,
                periodMs: input.periodMs
            }),
            expectedOutput,
            "PeriodicSubscriptionQos"
        );
        compareSubscriptionQosSettings(new OnChangeSubscriptionQos(input), expectedOutput, "OnChangeSubscriptionQos");
        compareSubscriptionQosSettings(
            new OnChangeWithKeepAliveSubscriptionQos({
                minIntervalMs: input.minIntervalMs,
                maxIntervalMs: input.maxIntervalMs,
                expiryDateMs: input.expiryDateMs,
                alertAfterIntervalMs: input.onChangeAlertAfterIntervalMs,
                publicationTtlMs: input.publicationTtlMs
            }),
            expectedOutput,
            "OnChangeWithKeepAliveSubscriptionQos"
        );
    }

    const defaultTestSettings = {
        expiryDateMs: 4,
        publicationTtlMs: 150,
        periodMs: 55,
        periodicAlertAfterIntervalMs: 80,
        minIntervalMs: 5,
        maxIntervalMs: 60,
        onChangeAlertAfterIntervalMs: 1000
    };

    // add non-existing keys in settings
    function addDefaultKeys(settings: any) {
        if (settings.expiryDateMs === undefined) {
            settings.expiryDateMs = defaultTestSettings.expiryDateMs;
        }
        if (settings.publicationTtlMs === undefined) {
            settings.publicationTtlMs = defaultTestSettings.publicationTtlMs;
        }
        if (settings.periodMs === undefined) {
            settings.periodMs = defaultTestSettings.periodMs;
        }
        if (settings.periodicAlertAfterIntervalMs === undefined) {
            settings.periodicAlertAfterIntervalMs = defaultTestSettings.periodicAlertAfterIntervalMs;
        }
        if (settings.minIntervalMs === undefined) {
            settings.minIntervalMs = defaultTestSettings.minIntervalMs;
        }
        if (settings.maxIntervalMs === undefined) {
            settings.maxIntervalMs = defaultTestSettings.maxIntervalMs;
        }
        if (settings.onChangeAlertAfterIntervalMs === undefined) {
            settings.onChangeAlertAfterIntervalMs = defaultTestSettings.onChangeAlertAfterIntervalMs;
        }
        return settings;
    }

    it("constructs SubscriptionQos (and subclasses) with correct member values", done => {
        // all values regular (lower limits)
        {
            const input = {
                expiryDateMs: SubscriptionQos.MIN_EXPIRY_MS,
                publicationTtlMs: SubscriptionQos.MIN_PUBLICATION_TTL_MS,
                periodMs: PeriodicSubscriptionQos.MIN_PERIOD_MS,
                periodicAlertAfterIntervalMs: PeriodicSubscriptionQos.MIN_PERIOD_MS,
                minIntervalMs: OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS,
                maxIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS,
                onChangeAlertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS
            };
            const expectedOutput = {
                expiryDateMs: SubscriptionQos.MIN_EXPIRY_MS,
                publicationTtlMs: SubscriptionQos.MIN_PUBLICATION_TTL_MS,
                periodMs: PeriodicSubscriptionQos.MIN_PERIOD_MS,
                periodicAlertAfterIntervalMs: PeriodicSubscriptionQos.MIN_PERIOD_MS,
                minIntervalMs: OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS,
                maxIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS,
                onChangeAlertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS
            };
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // all values regular (random values)
        {
            const input = addDefaultKeys({});
            const expectedOutput = addDefaultKeys({});
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // all values regular (upper limits)
        {
            const input = {
                expiryDateMs: UtilInternal.getMaxLongValue(),
                publicationTtlMs: SubscriptionQos.MAX_PUBLICATION_TTL_MS,
                periodMs: PeriodicSubscriptionQos.MAX_PERIOD_MS,
                periodicAlertAfterIntervalMs: PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS,
                minIntervalMs: OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                maxIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS,
                onChangeAlertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
            };
            const expectedOutput = {
                expiryDateMs: UtilInternal.getMaxLongValue(),
                publicationTtlMs: SubscriptionQos.MAX_PUBLICATION_TTL_MS,
                periodMs: PeriodicSubscriptionQos.MAX_PERIOD_MS,
                periodicAlertAfterIntervalMs: PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS,
                minIntervalMs: OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                maxIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS,
                onChangeAlertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
            };
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // publicationTtlMs < SubscriptionQos.MIN_PUBLICATION_TTL_MS
        {
            const input = addDefaultKeys({
                publicationTtlMs: SubscriptionQos.MIN_PUBLICATION_TTL_MS - 1
            });
            const expectedOutput = addDefaultKeys({
                publicationTtlMs: SubscriptionQos.MIN_PUBLICATION_TTL_MS
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // publicationTtlMs > SubscriptionQos.MAX_PUBLICATION_TTL_MS
        {
            const input = addDefaultKeys({
                publicationTtlMs: SubscriptionQos.MAX_PUBLICATION_TTL_MS + 1
            });
            const expectedOutput = addDefaultKeys({
                publicationTtlMs: SubscriptionQos.MAX_PUBLICATION_TTL_MS
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // expiryDateMs < SubscriptionQos.MIN_EXPIRY_MS
        {
            const input = addDefaultKeys({
                expiryDateMs: SubscriptionQos.MIN_EXPIRY_MS - 1
            });
            const expectedOutput = addDefaultKeys({
                expiryDateMs: SubscriptionQos.MIN_EXPIRY_MS
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // periodMs < PeriodicSubscriptionQos.MIN_PERIOD_MS
        expect(() => {
            // eslint-disable-next-line no-new
            new PeriodicSubscriptionQos(addDefaultKeys({ periodMs: PeriodicSubscriptionQos.MIN_PERIOD_MS - 1 }));
        }).toThrow();

        // periodMs > PeriodicSubscriptionQos.MAX_PERIOD_MS
        expect(() => {
            // eslint-disable-next-line no-new
            new PeriodicSubscriptionQos(addDefaultKeys({ periodMs: PeriodicSubscriptionQos.MAX_PERIOD_MS + 1 }));
        }).toThrow();

        // PeriodicSubscriptionQos.alertAfterIntervalMs !== PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL && PeriodicSubscriptionQos.alertAfterIntervalMs < periodMs
        {
            const input = addDefaultKeys({
                periodicAlertAfterIntervalMs: 2999,
                periodMs: 3000
            });
            const expectedOutput = addDefaultKeys({
                periodicAlertAfterIntervalMs: 3000,
                periodMs: 3000
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // PeriodicSubscriptionQos.alertAfterIntervalMs === PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL && PeriodicSubscriptionQos.alertAfterIntervalMs < periodMs
        {
            const input = addDefaultKeys({
                periodicAlertAfterIntervalMs: PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL,
                periodMs: PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL + 50
            });
            const expectedOutput = addDefaultKeys({
                periodicAlertAfterIntervalMs: PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL,
                periodMs: PeriodicSubscriptionQos.NO_ALERT_AFTER_INTERVAL + 50
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // PeriodicSubscriptionQos.alertAfterIntervalMs > PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
        {
            const input = addDefaultKeys({
                periodicAlertAfterIntervalMs: PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS + 1
            });
            const expectedOutput = addDefaultKeys({
                periodicAlertAfterIntervalMs: PeriodicSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // minIntervalMs < OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS
        {
            const input = addDefaultKeys({
                minIntervalMs: OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS - 1
            });
            const expectedOutput = addDefaultKeys({
                minIntervalMs: OnChangeSubscriptionQos.MIN_MIN_INTERVAL_MS
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // minIntervalMs > OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS
        {
            const input = addDefaultKeys({
                minIntervalMs: OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS + 1,
                maxIntervalMs: OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                onChangeAlertAfterIntervalMs: OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS
            });
            const expectedOutput = addDefaultKeys({
                minIntervalMs: OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                maxIntervalMs: OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS,
                onChangeAlertAfterIntervalMs: OnChangeSubscriptionQos.MAX_MIN_INTERVAL_MS
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // maxIntervalMs < OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS
        {
            const input = addDefaultKeys({
                maxIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS - 1
            });
            const expectedOutput = addDefaultKeys({
                maxIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MIN_MAX_INTERVAL_MS
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // maxIntervalMs > OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS
        {
            const input = addDefaultKeys({
                maxIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS + 1,
                onChangeAlertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS
            });
            const expectedOutput = addDefaultKeys({
                maxIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS,
                onChangeAlertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MAX_MAX_INTERVAL_MS
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // maxIntervalMs < minIntervalMs
        {
            const input = addDefaultKeys({ maxIntervalMs: 200, minIntervalMs: 500 });
            const expectedOutput = addDefaultKeys({
                maxIntervalMs: 500,
                minIntervalMs: 500
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // OnChangeWithKeepAliveSubscriptionQos.alertAfterIntervalMs !== OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL
        // && OnChangeWithKeepAliveSubscriptionQos.alertAfterIntervalMs < maxIntervalMs
        {
            const input = addDefaultKeys({
                maxIntervalMs: 4000,
                onChangeAlertAfterIntervalMs: 2000
            });
            const expectedOutput = addDefaultKeys({
                maxIntervalMs: 4000,
                onChangeAlertAfterIntervalMs: 4000
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // OnChangeWithKeepAliveSubscriptionQos.alertAfterIntervalMs === OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL
        // && OnChangeWithKeepAliveSubscriptionQos.alertAfterIntervalMs < maxIntervalMs
        {
            const input = addDefaultKeys({
                maxIntervalMs: OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL + 100,
                onChangeAlertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL
            });
            const expectedOutput = addDefaultKeys({
                maxIntervalMs: OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL + 100,
                onChangeAlertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.NO_ALERT_AFTER_INTERVAL
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        // OnChangeWithKeepAliveSubscriptionQos.alertAfterIntervalMs > OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
        {
            const input = addDefaultKeys({
                onChangeAlertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS + 1
            });
            const expectedOutput = addDefaultKeys({
                onChangeAlertAfterIntervalMs: OnChangeWithKeepAliveSubscriptionQos.MAX_ALERT_AFTER_INTERVAL_MS
            });
            checkSettingsInSubscriptionQosAndChildren(input, expectedOutput);
        }
        done();
    });

    it("constructs SubscriptionQos with correct default values", done => {
        const fixture = new SubscriptionQos();
        expect(fixture.expiryDateMs).toEqual(SubscriptionQos.NO_EXPIRY_DATE);
        expect(fixture.publicationTtlMs).toEqual(SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS);
        done();
    });

    it("constructs MulticastSubscriptionQos with correct default values", done => {
        const fixture = new MulticastSubscriptionQos();
        expect(fixture.expiryDateMs).toEqual(SubscriptionQos.NO_EXPIRY_DATE);
        expect(fixture.publicationTtlMs).toEqual(SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS);
        done();
    });

    it("constructs PeriodicSubscriptionQos with correct default values", done => {
        const fixture = new PeriodicSubscriptionQos();
        expect(fixture.periodMs).toEqual(PeriodicSubscriptionQos.DEFAULT_PERIOD_MS);
        expect(fixture.expiryDateMs).toEqual(SubscriptionQos.NO_EXPIRY_DATE);
        expect(fixture.alertAfterIntervalMs).toEqual(PeriodicSubscriptionQos.DEFAULT_ALERT_AFTER_INTERVAL_MS);
        expect(fixture.publicationTtlMs).toEqual(SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS);
        done();
    });

    it("constructs OnChangeSubscriptionQos with correct default values", done => {
        const fixture = new OnChangeSubscriptionQos();
        expect(fixture.expiryDateMs).toEqual(SubscriptionQos.NO_EXPIRY_DATE);
        expect(fixture.publicationTtlMs).toEqual(SubscriptionQos.DEFAULT_PUBLICATION_TTL_MS);
        expect(fixture.minIntervalMs).toEqual(OnChangeSubscriptionQos.DEFAULT_MIN_INTERVAL_MS);
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
        jest.spyOn(Date, "now").mockImplementation(() => {
            return fakeTime;
        });

        const fixture = new OnChangeWithKeepAliveSubscriptionQos({
            validityMs
        });
        expect(fixture.expiryDateMs).toBe(fakeTime + validityMs);
        done();
    });
});
