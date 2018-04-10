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
const GlobalDiscoveryEntry = require("../../../../main/js/generated/joynr/types/GlobalDiscoveryEntry");
const ProviderQos = require("../../../../main/js/generated/joynr/types/ProviderQos");
const ProviderScope = require("../../../../main/js/generated/joynr/types/ProviderScope");
const CustomParameter = require("../../../../main/js/generated/joynr/types/CustomParameter");
const Version = require("../../../../main/js/generated/joynr/types/Version");
const InProcessStub = require("../../../../main/js/joynr/util/InProcessStub");
const InProcessSkeleton = require("../../../../main/js/joynr/util/InProcessSkeleton");

describe("libjoynr-js.joynr.util.InProcessStubAndSkeleton", () => {
    it("InProcessSkeleton is instantiable", done => {
        expect(new InProcessSkeleton()).toBeDefined();
        expect(new InProcessSkeleton() instanceof InProcessSkeleton).toBeTruthy();
        done();
    });
});

describe("libjoynr-js.joynr.util.InProcessStubAndSkeleton", () => {
    it("InProcessStub is instantiable", done => {
        expect(new InProcessStub(new InProcessSkeleton())).toBeDefined();
        expect(new InProcessStub(new InProcessSkeleton()) instanceof InProcessStub).toBeTruthy();
        done();
    });
});

const capability = {
    discoveryEntry: new GlobalDiscoveryEntry({
        providerVersion: new Version({
            majorVersion: 47,
            minorVersion: 11
        }),
        domain: "KeywordmyDomain",
        interfaceName: "myInterfaceName",
        qos: new ProviderQos({
            customParameters: [
                new CustomParameter({
                    name: "theName",
                    value: "theValue"
                })
            ],
            priority: 1,
            scope: ProviderScope.LOCAL,
            supportsOnChangeSubscriptions: true
        }),
        address: "InProcessAddress",
        participantId: "1",
        publicKeyId: ""
    })
};

const arrayOfCapabilities = {
    discoveryEntries: [
        new GlobalDiscoveryEntry({
            providerVersion: new Version({
                majorVersion: 47,
                minorVersion: 11
            }),
            domain: "KeywordmyDomain",
            interfaceName: "myInterfaceName",
            qos: new ProviderQos({
                customParameters: [
                    new CustomParameter({
                        name: "theName",
                        value: "theValue"
                    })
                ],
                priority: 1,
                scope: ProviderScope.LOCAL,
                supportsOnChangeSubscriptions: true
            }),
            address: "InProcessAddress",
            participantId: "1",
            publicKeyId: ""
        }),
        new GlobalDiscoveryEntry({
            providerVersion: new Version({
                majorVersion: 47,
                minorVersion: 11
            }),
            domain: "myDomain",
            interfaceName: "myInterfaceName",
            qos: new ProviderQos({
                customParameters: [
                    new CustomParameter({
                        name: "theName",
                        value: "theValue"
                    })
                ],
                priority: 4,
                scope: ProviderScope.LOCAL,
                supportsOnChangeSubscriptions: true
            }),
            address: "InProcessAddress",
            participantId: "1",
            publicKeyId: ""
        }),
        new GlobalDiscoveryEntry({
            providerVersion: new Version({
                majorVersion: 47,
                minorVersion: 11
            }),
            domain: "myWithKeywordDomain",
            interfaceName: "myInterfaceName",
            qos: new ProviderQos({
                customParameters: [
                    new CustomParameter({
                        name: "theName",
                        value: "theValue"
                    })
                ],
                priority: 3,
                scope: ProviderScope.LOCAL,
                supportsOnChangeSubscriptions: true
            }),
            address: "InProcessAddress",
            participantId: "1",
            publicKeyId: ""
        }),
        new GlobalDiscoveryEntry({
            providerVersion: new Version({
                majorVersion: 47,
                minorVersion: 11
            }),
            domain: "myDomain",
            interfaceName: "myInterfaceNameKeyword",
            qos: new ProviderQos({
                customParameters: [
                    new CustomParameter({
                        name: "theName",
                        value: "theValue"
                    })
                ],
                priority: 5,
                scope: ProviderScope.LOCAL,
                supportsOnChangeSubscriptions: true
            }),
            address: "InProcessAddress",
            participantId: "1",
            publicKeyID: ""
        }),
        new GlobalDiscoveryEntry({
            providerVersion: new Version({
                majorVersion: 47,
                minorVersion: 11
            }),
            domain: "myDomain",
            interfaceName: "myInterfaceName",
            qos: new ProviderQos({
                customParameters: [
                    new CustomParameter({
                        name: "theName",
                        value: "theValue"
                    })
                ],
                priority: 2,
                scope: ProviderScope.LOCAL,
                supportsOnChangeSubscriptions: true
            }),
            address: "InProcessAddress",
            participantId: "1",
            publicKeyId: ""
        })
    ]
};

const providerQos = new ProviderQos({
    qos: [
        {
            name: "theName",
            value: "theValue",
            type: "QosString"
        }
    ],
    version: 123,
    priority: 1234
});

const myDomain = "myDomain";
const myInterfaceName = "myInterfaceName";

const participantId = {
    participandId: "participantId"
};

const participantIds = {
    participantIds: [participantId, "participantId2"]
};

describe("libjoynr-js.joynr.util.InProcessStubAndSkeleton", () => {
    function check(stub, spy) {
        const lookupTest = {
            domain: myDomain,
            interfaceName: myInterfaceName,
            providerQos
        };
        // make calls on the stub
        stub.remove(participantId);
        stub.remove(participantIds);
        stub.add(capability);
        stub.add(arrayOfCapabilities);
        stub.lookup(lookupTest);
        stub.lookup(participantId);

        // check if calls are going through to the mocked object
        expect(spy.remove.calls.argsFor(0)[0]).toEqual(participantId);
        expect(spy.remove.calls.argsFor(1)[0]).toEqual(participantIds);
        expect(spy.add.calls.argsFor(0)[0]).toEqual(capability);
        expect(spy.add.calls.argsFor(1)[0]).toEqual(arrayOfCapabilities);
        expect(spy.lookup.calls.argsFor(0)[0]).toEqual(lookupTest);
        expect(spy.lookup.calls.argsFor(1)[0]).toEqual(participantId);
    }

    let spy;
    beforeEach(() => {
        // create mock object for capabilities directory
        spy = jasmine.createSpyObj("capabilitiesDirectory", ["remove", "add", "lookup"]);
    });

    it("all methods get called through stub and skeleton correctly", done => {
        const stub = new InProcessStub(new InProcessSkeleton(spy));
        check(stub, spy);
        done();
    });

    it("all methods get called through stub and skeleton correctly with late initialization", done => {
        const stub = new InProcessStub();
        stub.setSkeleton(new InProcessSkeleton(spy));
        check(stub, spy);
        done();
    });

    it("all methods get called through stub and skeleton correctly after overwrite", done => {
        const stub = new InProcessStub();
        stub.setSkeleton(
            new InProcessSkeleton({
                someKey: "someValue"
            })
        );
        stub.setSkeleton(new InProcessSkeleton(spy));
        check(stub, spy);
        done();
    });

    const objects = [
        {
            key: "value"
        },
        0,
        -1,
        1234,
        "",
        "a strting",
        new InProcessStub(new InProcessSkeleton({}))
    ];

    function testValue(obj) {
        expect(() => new InProcessStub(obj)).toThrow();
        expect(() => new InProcessStub().setSkeleton(obj)).toThrow();
    }

    it("throws when Stub receives object which is not of type InProcessSkeleton", done => {
        let i;
        for (i = 0; i < objects.length; ++i) {
            testValue(objects[i]);
        }
        expect(() => new InProcessStub()).not.toThrow();
        done();
    });

    it("throws when inProcessSkeleton is undefined or null ", done => {
        expect(() => {
            new InProcessStub().setSkeleton(undefined);
        }).toThrow();
        expect(() => {
            new InProcessStub().setSkeleton(null);
        }).toThrow();
        done();
    });
});
