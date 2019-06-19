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

import ProxyOperation from "../../../../main/js/joynr/proxy/ProxyOperation";
import MessagingQos from "../../../../main/js/joynr/messaging/MessagingQos";
import * as Request from "../../../../main/js/joynr/dispatching/types/Request";
import * as OneWayRequest from "../../../../main/js/joynr/dispatching/types/OneWayRequest";
import TypeRegistrySingleton from "../../../../main/js/joynr/types/TypeRegistrySingleton";
import testDataOperation from "../../../../test/js/test/data/Operation";
import TestEnum from "../../../generated/joynr/tests/testTypes/TestEnum";
import RadioStation from "../../../generated/joynr/vehicle/radiotypes/RadioStation";
import DiscoveryEntryWithMetaInfo from "../../../../main/js/generated/joynr/types/DiscoveryEntryWithMetaInfo";
import Version from "../../../../main/js/generated/joynr/types/Version";
import ProviderQos from "../../../../main/js/generated/joynr/types/ProviderQos";

describe("libjoynr-js.joynr.proxy.ProxyOperation", () => {
    let addFavoriteStation: Function;
    let operationName: any;
    let proxyParticipantId: any;
    let providerParticipantId: any;
    let providerDiscoveryEntry: any;
    let proxy: any;
    let requestReplyManagerSpy: any;

    beforeEach(() => {
        requestReplyManagerSpy = {
            sendRequest: jest.fn(),
            sendOneWayRequest: jest.fn()
        };
        requestReplyManagerSpy.sendRequest.mockImplementation((_settings: any, callbackSettings: any) => {
            const response = { result: "resultValue" };

            return Promise.resolve({
                response,
                settings: callbackSettings
            });
        });

        operationName = "myOperation";
        proxyParticipantId = "proxyParticipantId";
        providerParticipantId = "providerParticipantId";
        providerDiscoveryEntry = new DiscoveryEntryWithMetaInfo({
            providerVersion: new Version({ majorVersion: 0, minorVersion: 23 }),
            domain: "testProviderDomain",
            interfaceName: "interfaceName",
            participantId: providerParticipantId,
            qos: new ProviderQos(undefined as any),
            lastSeenDateMs: Date.now(),
            expiryDateMs: Date.now() + 60000,
            publicKeyId: "publicKeyId",
            isLocal: true
        });
        proxy = {
            proxyParticipantId,
            providerDiscoveryEntry,
            settings: {
                dependencies: {
                    requestReplyManager: requestReplyManagerSpy
                }
            },
            messagingQos: new MessagingQos()
        };

        addFavoriteStation = new ProxyOperation(proxy, "addFavoriteStation", [
            {
                inputParameter: [
                    {
                        name: "radioStation",
                        type: "joynr.vehicle.radiotypes.RadioStation"
                    }
                ]
            },
            {
                inputParameter: [
                    {
                        name: "radioStation",
                        type: "String"
                    }
                ],
                outputParameter: []
            }
        ]).buildFunction();

        TypeRegistrySingleton.getInstance().addType(TestEnum);
    });

    it("is of correct type", done => {
        expect(addFavoriteStation).toBeDefined();
        expect(typeof addFavoriteStation === "function").toBeTruthy();
        done();
    });

    it("expect correct error reporting after operation call with wrong argument", done => {
        addFavoriteStation({
            nonexistingArgument: "value"
        })
            .then(() => done.fail())
            .catch((error: any) => {
                expect(error.message).toMatch("Cannot call operation with nullable value");
                done();
            });
    });

    it("expect correct error reporting after operation call with wrong type of argument", done => {
        addFavoriteStation({
            radioStation: 1
        })
            .then(() => done.fail())
            .catch((error: any) => {
                //expect(message).toContain(
                //    "Signature does not match");
                expect(error.message).toMatch("Signature does not match");
                done();
            });
    });

    it("expect no error reporting after operation call with correct string argument", async () => {
        await expect(
            addFavoriteStation({
                radioStation: "correctValue"
            })
        ).resolves.toBeUndefined();
    });

    async function testForCorrectReturnValues(
        methodName: any,
        outputParameter: any,
        replyResponse: any,
        expected?: any
    ) {
        const proxy = {
            proxyParticipantId,
            providerDiscoveryEntry,
            messagingQos: new MessagingQos(),
            settings: { dependencies: { requestReplyManager: requestReplyManagerSpy } }
        };

        requestReplyManagerSpy.sendRequest.mockImplementation((_settings: any, callbackSettings: any) => {
            return Promise.resolve({
                response: replyResponse,
                settings: callbackSettings
            });
        });

        const testMethod = new ProxyOperation(proxy, methodName, [
            {
                inputParameter: [],
                outputParameter
            }
        ]).buildFunction();

        const result = await testMethod();
        expect(result).toEqual(expected);
    }
    it("expect correct joynr enum object as return value", () => {
        return testForCorrectReturnValues(
            "testMethodHavingEnumAsReturnValue",
            [
                {
                    name: "returnEnum",
                    type: TestEnum.ZERO._typeName
                }
            ],
            ["ZERO"],
            {
                returnEnum: TestEnum.ZERO
            }
        );
    });

    it("expect undefined as return value for missing output parameters", () => {
        return testForCorrectReturnValues("testMethodHavingNoOutputParameter", [], [], undefined)
            .then(() =>
                testForCorrectReturnValues("testMethodHavingNoOutputParameter", [], ["unexpected value"], undefined)
            )
            .then(() => testForCorrectReturnValues("testMethodWithUndefinedOutputParameter", undefined, [], undefined));
    });

    it("expect multiple return values", done => {
        testForCorrectReturnValues(
            "testMultipleReturnValues",
            [
                {
                    name: "returnEnum",
                    type: TestEnum.ZERO._typeName
                },
                {
                    name: "returnString",
                    type: "String"
                }
            ],
            ["ZERO", "stringValue"],
            {
                returnEnum: TestEnum.ZERO,
                returnString: "stringValue"
            }
        )
            .then(() => {
                done();
                return null;
            })
            .catch(() => done.fail());
    });

    it("expect correct joynr enum object array as return value", () => {
        return testForCorrectReturnValues(
            "testMethodHavingEnumArrayAsReturnValue",
            [
                {
                    name: "returnEnum",
                    // currently, we generate the type of the array element into the signature
                    type: `${TestEnum.ZERO._typeName}[]`
                }
            ],
            [["ZERO", "ONE"]],
            {
                returnEnum: [TestEnum.ZERO, TestEnum.ONE]
            }
        );
    });

    it("expect no error reporting after operation call with correct complex argument", async () => {
        const radioStation = new RadioStation({
            name: "correctValue",
            byteBuffer: []
        });

        const returnValue = await addFavoriteStation({
            radioStation
        });
        expect(returnValue).toEqual(undefined);
    });

    it("notifies", () => {
        return addFavoriteStation({
            radioStation: "stringStation"
        });
    });

    async function testOperationOverloading(operationArguments: any, errorExpected?: boolean) {
        if (errorExpected) {
            await expect(addFavoriteStation(operationArguments)).rejects.toBeInstanceOf(Error);
        } else {
            await expect(addFavoriteStation(operationArguments));
        }
    }

    it("provides overloading operations", done => {
        // correct version one
        testOperationOverloading({
            radioStation: "stringStation"
        })
            .then(() => {
                return testOperationOverloading({
                    radioStation: new RadioStation({
                        name: "typedStation",
                        byteBuffer: []
                    })
                }); // correct version two
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        wrongName: "stringStation"
                    },
                    true
                ); // wrong argument name
            })
            .then(() => {
                return testOperationOverloading({}, true); // wrong number of arguments
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        radioStation: []
                    },
                    true
                ); // wrong number argument type (Array instead of String|RadioStation)
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        radioStation: 1
                    },
                    true
                ); // wrong number argument type (Number instead of String|RadioStation)
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        radioStation: "stringStation",
                        anotherArgument: 1
                    },
                    true
                ); // wrong additional argument
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        radioStation: new RadioStation({
                            name: "stringStation",
                            byteBuffer: []
                        }),
                        anotherArgument: 2
                    },
                    true
                ); // wrong additional arguments
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        radioStation: null
                    },
                    true
                ); // nullable argument
            })
            .then(() => {
                return testOperationOverloading(
                    {
                        radioStation: undefined
                    },
                    true
                ); // nullable argument
            })
            .then(() => {
                return testOperationOverloading(undefined, true); // nullable settings object
            })
            .then(() => {
                return testOperationOverloading(null, true); // nullable settings object
            })
            .then(() => {
                done();
                return null;
            })
            .catch(() => done.fail());
    });

    it("does not throw when giving wrong or nullable operation arguments", done => {
        const spy = {
            onFulfilled: jest.fn(),
            onRejected: jest.fn()
        };
        expect(() => {
            addFavoriteStation({
                radioStation: "myRadioStation"
            })
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(() => {
            addFavoriteStation({
                radioStation: undefined
            })
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(() => {
            addFavoriteStation({})
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(() => {
            addFavoriteStation(undefined)
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        expect(() => {
            addFavoriteStation(null)
                .then(spy.onFulfilled)
                .catch(spy.onRejected);
        }).not.toThrow();

        done();
    });

    function checkRequestReplyManagerCall(testData: any) {
        // construct new ProxyOperation
        const myOperation = new ProxyOperation(proxy, operationName, [testData.signature]).buildFunction();

        requestReplyManagerSpy.sendRequest.mockImplementation((_settings: any, callbackSettings: any) => {
            return Promise.resolve({
                response: testData.returnParams,
                settings: callbackSettings
            });
        });
        requestReplyManagerSpy.sendRequest.mockClear();

        // do operation call
        myOperation(testData.namedArguments)
            .then(() => {
                // check if requestReplyManager has been called correctly
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();

                const requestReplyId = requestReplyManagerSpy.sendRequest.calls.argsFor(0)[0].request.requestReplyId;
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalledWith(
                    {
                        toDiscoveryEntry: providerDiscoveryEntry,
                        from: proxyParticipantId,
                        messagingQos: new MessagingQos(),
                        request: Request.create({
                            methodName: operationName,
                            paramDatatypes: testData.paramDatatypes,
                            params: testData.params,
                            requestReplyId
                        })
                    },
                    expect.any(Object)
                );
            })
            .catch(() => {
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
            });
    }

    function checkRequestReplyManagerFireAndForgetCall(testData: any) {
        const myOperation = new ProxyOperation(proxy, operationName, [testData.signature]).buildFunction();

        requestReplyManagerSpy.sendOneWayRequest.mockReturnValue(Promise.resolve());
        requestReplyManagerSpy.sendOneWayRequest.mockClear();

        // do operation call
        myOperation(testData.namedArguments)
            .then(() => {
                // check if requestReplyManager has been called correctly
                expect(requestReplyManagerSpy.sendOneWayRequest).toHaveBeenCalled();
                expect(requestReplyManagerSpy.sendOneWayRequest).toHaveBeenCalledWith({
                    toDiscoveryEntry: providerDiscoveryEntry,
                    from: proxyParticipantId,
                    messagingQos: new MessagingQos(),
                    request: OneWayRequest.create({
                        methodName: operationName,
                        paramDatatypes: testData.paramDatatypes,
                        params: testData.params
                    })
                });
            })
            .catch(() => {
                expect(requestReplyManagerSpy.sendRequest).toHaveBeenCalled();
            });
    }

    it("calls RequestReplyManager with correct request", async () => {
        if (testDataOperation[0].signature.fireAndForget) {
            await checkRequestReplyManagerFireAndForgetCall(testDataOperation[0]);
        } else {
            await checkRequestReplyManagerCall(testDataOperation[0]);
        }

        for (let i = 1; i < testDataOperation.length; ++i) {
            const testOp = testDataOperation[i];
            if (testOp.signature.fireAndForget) {
                await checkRequestReplyManagerFireAndForgetCall(testOp);
            } else {
                await checkRequestReplyManagerCall(testOp);
            }
        }
        return;
    });
});
