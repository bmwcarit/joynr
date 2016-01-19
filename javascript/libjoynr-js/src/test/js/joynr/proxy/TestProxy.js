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

joynrTestRequire("joynr/proxy/TestProxy", [
    "global/Promise",
    "joynr/vehicle/RadioProxy",
    "joynr/vehicle/radiotypes/RadioStation",
    "joynr/proxy/ProxyAttributeNotifyReadWrite",
    "joynr/proxy/ProxyAttributeNotifyRead",
    "joynr/proxy/ProxyAttributeNotifyWrite",
    "joynr/proxy/ProxyAttributeNotify",
    "joynr/proxy/ProxyAttributeReadWrite",
    "joynr/proxy/ProxyAttributeRead",
    "joynr/proxy/ProxyAttributeWrite",
    "joynr/proxy/ProxyOperation",
    "joynr/proxy/ProxyEvent",
    "joynr/types/TypeRegistrySingleton",
    "joynr/proxy/DiscoveryQos",
    "joynr/messaging/MessagingQos"
], function(
        Promise,
        RadioProxy,
        RadioStation,
        ProxyAttributeNotifyReadWrite,
        ProxyAttributeNotifyRead,
        ProxyAttributeNotifyWrite,
        ProxyAttributeNotify,
        ProxyAttributeReadWrite,
        ProxyAttributeRead,
        ProxyAttributeWrite,
        ProxyOperation,
        ProxyEvent,
        TypeRegistrySingleton,
        DiscoveryQos,
        MessagingQos) {

    describe("libjoynr-js.joynr.proxy.Proxy", function() {

        var settings, dependencies, radioProxy;
        var typeRegistry = TypeRegistrySingleton.getInstance();

        beforeEach(function() {
            settings = {
                domain : "",
                interfaceName : "",
                discoveryQos : new DiscoveryQos(),
                messagingQos : new MessagingQos(),
                proxyElementTypes : {
                    ProxyAttributeNotifyReadWrite : ProxyAttributeNotifyReadWrite,
                    ProxyAttributeNotifyRead : ProxyAttributeNotifyRead,
                    ProxyAttributeNotifyWrite : ProxyAttributeNotifyWrite,
                    ProxyAttributeNotify : ProxyAttributeNotify,
                    ProxyAttributeReadWrite : ProxyAttributeReadWrite,
                    ProxyAttributeRead : ProxyAttributeRead,
                    ProxyAttributeWrite : ProxyAttributeWrite,
                    ProxyOperation : ProxyOperation,
                    ProxyEvent : ProxyEvent
                },
                dependencies : {
                    subscriptionManager : {}
                }
            };
            radioProxy = new RadioProxy(settings);
        });

        it("RadioProxy is instantiable", function() {
            expect(radioProxy).toBeDefined();
            expect(radioProxy).not.toBeNull();
            expect(typeof radioProxy === "object").toBeTruthy();
            expect(radioProxy instanceof RadioProxy).toBeTruthy();
        });

        it("RadioProxy saves settings object", function() {
            expect(radioProxy.settings).toEqual(settings);
        });

        it("RadioProxy has all members", function() {
            expect(radioProxy.isOn).toBeDefined();
            expect(radioProxy.isOn instanceof ProxyAttributeNotifyReadWrite).toBeTruthy();
            expect(radioProxy.addFavoriteStation).toBeDefined();
            expect(typeof radioProxy.addFavoriteStation === "function").toBeTruthy();
            expect(radioProxy.weakSignal).toBeDefined();
            expect(radioProxy.weakSignal instanceof ProxyEvent).toBeTruthy();
        });

    });

}); // require
