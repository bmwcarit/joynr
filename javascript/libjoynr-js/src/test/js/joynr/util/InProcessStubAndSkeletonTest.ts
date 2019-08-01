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

import GlobalDiscoveryEntry from "../../../../main/js/generated/joynr/types/GlobalDiscoveryEntry";
import ProviderQos from "../../../../main/js/generated/joynr/types/ProviderQos";
import ProviderScope from "../../../../main/js/generated/joynr/types/ProviderScope";
import CustomParameter from "../../../../main/js/generated/joynr/types/CustomParameter";
import Version from "../../../../main/js/generated/joynr/types/Version";
import InProcessStub from "../../../../main/js/joynr/util/InProcessStub";
import InProcessSkeleton from "../../../../main/js/joynr/util/InProcessSkeleton";

const lastSeenDateMs = 0;
const expiryDateMs = 1e10;

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
        publicKeyId: "",
        lastSeenDateMs,
        expiryDateMs
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
            publicKeyId: "",
            lastSeenDateMs,
            expiryDateMs
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
            publicKeyId: "",
            lastSeenDateMs,
            expiryDateMs
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
            publicKeyId: "",
            lastSeenDateMs,
            expiryDateMs
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
            publicKeyId: "",
            lastSeenDateMs,
            expiryDateMs
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
            publicKeyId: "",
            lastSeenDateMs,
            expiryDateMs
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
    priority: 1234
} as any);

const myDomain = "myDomain";
const myInterfaceName = "myInterfaceName";

const participantId = {
    participandId: "participantId"
};

const participantIds = {
    participantIds: [participantId, "participantId2"]
};

describe("libjoynr-js.joynr.util.InProcessStubAndSkeleton", () => {
    function check(stub: any, spy: any) {
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
        expect(spy.remove.mock.calls[0][0]).toEqual(participantId);
        expect(spy.remove.mock.calls[1][0]).toEqual(participantIds);
        expect(spy.add.mock.calls[0][0]).toEqual(capability);
        expect(spy.add.mock.calls[1][0]).toEqual(arrayOfCapabilities);
        expect(spy.lookup.mock.calls[0][0]).toEqual(lookupTest);
        expect(spy.lookup.mock.calls[1][0]).toEqual(participantId);
    }

    let spy: any;
    beforeEach(() => {
        // create mock object for capabilities directory
        spy = {
            remove: jest.fn(),
            add: jest.fn(),
            lookup: jest.fn()
        };
    });

    it("InProcessSkeleton is instantiable", () => {
        expect(new InProcessSkeleton(undefined as any)).toBeDefined();
        expect(new InProcessSkeleton(undefined as any) instanceof InProcessSkeleton).toBeTruthy();
    });
    it("InProcessStub is instantiable", () => {
        expect(new InProcessStub(new InProcessSkeleton(undefined as any))).toBeDefined();
        expect(new InProcessStub(new InProcessSkeleton(undefined as any)) instanceof InProcessStub).toBeTruthy();
    });

    it("all methods get called through stub and skeleton correctly", () => {
        const stub = new InProcessStub(new InProcessSkeleton(spy));
        check(stub, spy);
    });

    it("all methods get called through stub and skeleton correctly with late initialization", () => {
        const stub = new InProcessStub();
        stub.setSkeleton(new InProcessSkeleton(spy));
        check(stub, spy);
    });

    it("all methods get called through stub and skeleton correctly after overwrite", () => {
        const stub = new InProcessStub();
        stub.setSkeleton(
            new InProcessSkeleton({
                someKey: "someValue"
            })
        );
        stub.setSkeleton(new InProcessSkeleton(spy));
        check(stub, spy);
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

    function testValue(obj: any) {
        expect(() => new InProcessStub(obj)).toThrow();
        expect(() => new InProcessStub().setSkeleton(obj)).toThrow();
    }

    it("throws when Stub receives object which is not of type InProcessSkeleton", () => {
        for (let i = 0; i < objects.length; ++i) {
            testValue(objects[i]);
        }
        expect(() => new InProcessStub()).not.toThrow();
    });
});
