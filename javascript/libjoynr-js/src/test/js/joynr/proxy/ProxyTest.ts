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

import RadioProxy from "../../../generated/joynr/vehicle/RadioProxy";
import ProxyEvent from "../../../../main/js/joynr/proxy/ProxyEvent";
import TypeRegistrySingleton from "../../../../main/js/joynr/types/TypeRegistrySingleton";
import DiscoveryQos from "../../../../main/js/joynr/proxy/DiscoveryQos";
import MessagingQos from "../../../../main/js/joynr/messaging/MessagingQos";
import TestWithVersionProxy from "../../../generated/joynr/tests/TestWithVersionProxy";
import TestWithoutVersionProxy from "../../../generated/joynr/tests/TestWithoutVersionProxy";

describe("libjoynr-js.joynr.proxy.Proxy", () => {
    let settings: any, radioProxy: any;
    const typeRegistry = TypeRegistrySingleton.getInstance();

    beforeEach(done => {
        settings = {
            domain: "",
            interfaceName: "",
            discoveryQos: new DiscoveryQos(),
            messagingQos: new MessagingQos(),
            dependencies: {
                subscriptionManager: {}
            }
        };
        radioProxy = new RadioProxy(settings);
        done();
    });

    it("version is set correctly", done => {
        expect(TestWithVersionProxy.MAJOR_VERSION).toBeDefined();
        expect(TestWithVersionProxy.MAJOR_VERSION).toEqual(47);
        expect(TestWithVersionProxy.MINOR_VERSION).toBeDefined();
        expect(TestWithVersionProxy.MINOR_VERSION).toEqual(11);
        done();
    });

    it("default version is set correctly", done => {
        expect(TestWithoutVersionProxy.MAJOR_VERSION).toBeDefined();
        expect(TestWithoutVersionProxy.MAJOR_VERSION).toEqual(0);
        expect(TestWithoutVersionProxy.MINOR_VERSION).toBeDefined();
        expect(TestWithoutVersionProxy.MINOR_VERSION).toEqual(0);
        done();
    });

    it("RadioProxy is instantiable", done => {
        expect(radioProxy).toBeDefined();
        expect(radioProxy).not.toBeNull();
        expect(typeof radioProxy === "object").toBeTruthy();
        expect(radioProxy instanceof RadioProxy).toBeTruthy();
        done();
    });

    it("RadioProxy provides API to access used datatypes", done => {
        expect(RadioProxy.getUsedJoynrtypes).toBeDefined();
        done();
    });

    it("RadioProxy.getUsedJoynrtypes can be used to register all used datatypes", () => {
        expect(RadioProxy.getUsedJoynrtypes).toBeDefined();
        expect(() =>
            RadioProxy.getUsedJoynrtypes().map((datatype: any) => {
                return typeRegistry.addType(datatype);
            })
        ).not.toThrow();
    });

    it("RadioProxy saves settings object", done => {
        expect(radioProxy.settings).toEqual(settings);
        done();
    });

    it("RadioProxy has all members", done => {
        expect(radioProxy.isOn).toBeDefined();
        expect(radioProxy.addFavoriteStation).toBeDefined();
        expect(typeof radioProxy.addFavoriteStation === "function").toBeTruthy();
        expect(radioProxy.weakSignal).toBeDefined();
        expect(radioProxy.weakSignal instanceof ProxyEvent).toBeTruthy();
        done();
    });
});
