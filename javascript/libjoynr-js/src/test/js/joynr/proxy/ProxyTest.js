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
const Promise = require("../../../../main/js/global/Promise");
const RadioProxy = require("../../../generated/joynr/vehicle/RadioProxy");
const RadioStation = require("../../../generated/joynr/vehicle/radiotypes/RadioStation");
const ProxyAttribute = require("../../../../main/js/joynr/proxy/ProxyAttribute");
const ProxyOperation = require("../../../../main/js/joynr/proxy/ProxyOperation");
const ProxyEvent = require("../../../../main/js/joynr/proxy/ProxyEvent");
const TypeRegistrySingleton = require("../../../../main/js/joynr/types/TypeRegistrySingleton");
const DiscoveryQos = require("../../../../main/js/joynr/proxy/DiscoveryQos");
const MessagingQos = require("../../../../main/js/joynr/messaging/MessagingQos");
const TestWithVersionProxy = require("../../../generated/joynr/tests/TestWithVersionProxy");
const TestWithoutVersionProxy = require("../../../generated/joynr/tests/TestWithoutVersionProxy");

describe("libjoynr-js.joynr.proxy.Proxy", () => {
    let settings, dependencies, radioProxy;
    const typeRegistry = TypeRegistrySingleton.getInstance();

    beforeEach(done => {
        settings = {
            domain: "",
            interfaceName: "",
            discoveryQos: new DiscoveryQos(),
            messagingQos: new MessagingQos(),
            proxyElementTypes: {
                ProxyAttribute,
                ProxyOperation,
                ProxyEvent
            },
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
        expect(RadioProxy.getUsedDatatypes).toBeDefined();
        done();
    });

    it("RadioProxy.getUsedDatatype can be used to synchronize to the successful registration of all used datatypes", done => {
        let datatypePromises;
        let allDatatypesRegistered;
        allDatatypesRegistered = false;
        expect(RadioProxy.getUsedDatatypes).toBeDefined();
        datatypePromises = RadioProxy.getUsedDatatypes().map(datatype => {
            return typeRegistry.getTypeRegisteredPromise(datatype, 1000);
        });
        Promise.all(datatypePromises)
            .then(() => {
                done();
                return null;
            })
            .catch(error => {
                fail("failed to register all datatypes at the typeRegistry");
                return null;
            });
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
