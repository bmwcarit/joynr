/*global joynrTestRequire: true */

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

//TODO: some of this relies on the dummy implementation, change accordingly when implementating
joynrTestRequire("joynr/provider/TestProviderEvent", [
    "joynr/provider/ProviderEvent",
    "joynr/types/ProviderQos"
], function(ProviderEvent, ProviderQos) {

    var safetyTimeoutDelta = 100;

    describe("libjoynr-js.joynr.provider.ProviderEvent", function() {

        var settings;
        var weakSignal;
        var weakSignalNotifyReadOnly;
        var weakSignalNotifyWriteOnly;
        var weakSignalNotify;
        var weakSignalReadWrite;
        var weakSignalReadOnly, weakSignalWriteOnly;
        var weakSignalProviderEventNotifyReadWrite;
        var weakSignalProviderEventNotifyRead;
        var weakSignalProviderEventNotifyWrite;
        var weakSignalProviderEventNotify;
        var weakSignalProviderEventReadWrite;
        var weakSignalProviderEventRead;
        var weakSignalProviderEventWrite;

        beforeEach(function() {
            settings = {
                providerQos : new ProviderQos({
                    version : 123,
                    priority : 1234
                })
            };
            weakSignal = new ProviderEvent(settings, "weakSignal");
        });

        it("is of correct type", function() {
            expect(weakSignal).toBeDefined();
            expect(weakSignal).not.toBeNull();
            expect(typeof weakSignal === "object").toBeTruthy();
            expect(weakSignal instanceof ProviderEvent).toBeTruthy();
        });

        it("value changed works", function() {
            expect(weakSignal.valueChanged).toBeDefined();
            expect(typeof weakSignal.valueChanged === "function").toBeTruthy();
            expect(function() {
                weakSignal.valueChanged(Math.random() >= 0.5);
            }).not.toThrow();
        });

    });

}); // require
