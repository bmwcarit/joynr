/*global joynrTestRequire: true, requiresJsUndef: true */
/*jslint es5: true */

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

joynrTestRequire(
        "integration/TestInterTab",
        [
            "global/Promise",
            "joynr",
            "joynr/types/DiscoveryEntry",
            "joynr/types/DiscoveryScope",
            "joynr/types/DiscoveryQos",
            "joynr/system/DiscoveryProxy",
            "joynr/system/RoutingProxy",
            "joynr/system/RoutingProvider",
            "joynr/system/RoutingTypes/ChannelAddress",
            "joynr/system/RoutingTypes/BrowserAddress",
            "joynr/system/RoutingTypes/WebSocketAddress",
            "joynr/system/RoutingTypes/CommonApiDbusAddress",
            "joynr/vehicle/RadioProxy",
            "joynr/vehicle/RadioProvider",
            "joynr/vehicle/radiotypes/RadioStation",
            "joynr/datatypes/exampleTypes/Country",
            "joynr/datatypes/exampleTypes/StringMap",
            "joynr/provisioning/provisioning_libjoynr",
            "integration/IntegrationUtils"
        ],
        function(
                Promise,
                joynr,
                DiscoveryEntry,
                DiscoveryScope,
                DiscoveryQos,
                DiscoveryProxy,
                RoutingProxy,
                RoutingProvider,
                ChannelAddress,
                BrowserAddress,
                WebSocketAddress,
                CommonApiDbusAddress,
                RadioProxy,
                RadioProvider,
                RadioStation,
                Country,
                StringMap,
                provisioning,
                IntegrationUtils) {
            describe(
                    "libjoynr-js.integration.intertab",
                    function() {
                        var parentWindow;
                        var libjoynrParentWindow;
                        var radioProvider;
                        var radioProxy;
                        var workerStarted, libjoynrLoaded;
                        var numberOfStations;
                        var provisioningSuffix;
                        var discoveryTimeout;
                        var messagingQos;

                        beforeEach(function() {
                            var testProvisioning;
                            var worker;
                            workerStarted = false;
                            libjoynrLoaded = false;
                            var webWorkerAndLibJoynrStarted = false;
                            provisioningSuffix = "-" + Date.now();
                            discoveryTimeout = 4000;

                            /*
                             * The parent window is used by libjoynr to postMessages in case of
                             * intertab communication For testing purposes, we immitate the parent
                             * window, and directly forward the messages to be sent to the webworker
                             * of interest The webworker itself implements the counterpart of this
                             * "tunnel", by immitating the window on web worker side and forwarding
                             * incoming messages to all event listeners
                             */
                            libjoynrParentWindow =
                                    {
                                        time : Date.now(),
                                        workerId : undefined,
                                        postMessage : function(event, origin) {
                                            worker = IntegrationUtils.getCCWorker();
                                            if (worker) {
                                                worker.postMessage({
                                                    type : "message",
                                                    message : event.message
                                                });
                                                IntegrationUtils.log({
                                                    level: "debug",
                                                    message: "post message for cluster controller with ID "
                                                        + libjoynrParentWindow.workerId
                                                }, "joynr.integration.TestInterTab");
                                            } else {
                                                IntegrationUtils.log({
                                                    level: "debug",
                                                    message: "ATTENTION: worker for cluster controller with ID "
                                                        + libjoynrParentWindow.workerId
                                                        + " could not be resolved. Aborting message post..."
                                                }, "joynr.integration.TestInterTab");
                                            }
                                        }
                                    };
                            provisioning.parentWindow = libjoynrParentWindow;
                            provisioning.windowId = "libjoynr_" + libjoynrParentWindow.time;

                            testProvisioning = IntegrationUtils.getProvisioning(provisioning,provisioningSuffix);

                            runs(function() {
                                IntegrationUtils
                                        .initializeWebWorkerCC(
                                                "TestInterTabCommunicationCCWorker",
                                                provisioningSuffix)
                                        .then(
                                                function(newWorkerId) {
                                                    libjoynrParentWindow.workerId = newWorkerId;
                                                    IntegrationUtils.log({
                                                        level: "debug",
                                                        message: "CC web worker with ID: initialized with id "
                                                                     + libjoynrParentWindow.workerId
                                                    }, "joynr.integration.TestInterTab");
                                                    IntegrationUtils.log({
                                                        level: "debug",
                                                        message: "CC web worker with ID " + libjoynrParentWindow.workerId
                                                                + ": starting"
                                                    }, "joynr.integration.TestInterTab");
                                                    IntegrationUtils.startWebWorker(libjoynrParentWindow.workerId).then(function() {
                                                        workerStarted = true;
                                                        IntegrationUtils.log({
                                                            level: "debug",
                                                            message: "CC web worker with ID " + libjoynrParentWindow.workerId
                                                                    + ": started"
                                                        }, "joynr.integration.TestInterTab");
                                                        joynr.load(testProvisioning, true).then(function(newJoynr){
                                                            messagingQos = IntegrationUtils.messagingQos;
                                                            joynr = newJoynr;
                                                            IntegrationUtils.initialize(joynr);
                                                            libjoynrLoaded = true;
                                                        }).catch(function(error){
                                                            throw error;
                                                        });
                                                    });
                                                });
                            });

                            waitsFor(function() {
                                return workerStarted && libjoynrLoaded;
                            }, "worker started", testProvisioning.ttl);
                        });

                        function shutdownWebWorkerAndLibJoynr() {
                            var shutDownWW;
                            var shutDownLibJoynr;
                            radioProxy = undefined;

                            runs(function() {
                                IntegrationUtils.log({
                                    level: "debug",
                                    message: "CC web worker with ID " + libjoynrParentWindow.workerId
                                            + ": terminating"
                                }, "joynr.integration.TestInterTab");
                                IntegrationUtils.shutdownWebWorker(libjoynrParentWindow.workerId)
                                        .then(
                                                function() {
                                                    shutDownWW = true;
                                                    IntegrationUtils.log({
                                                        level: "debug",
                                                        message: "CC web worker with ID " + libjoynrParentWindow.workerId
                                                                + ": terminated"
                                                    }, "joynr.integration.TestInterTab");
                                                });
                                IntegrationUtils.shutdownLibjoynr().then(function() {
                                    delete provisioning.parentWindow;
                                    shutDownLibJoynr = true;
                                });
                            });

                            waitsFor(function() {
                                return shutDownWW && shutDownLibJoynr;
                            }, "WebWorker and Libjoynr to be shut down", 5000);
                        }

                        afterEach(function() {
                            shutdownWebWorkerAndLibJoynr();
                        });

                        function createDiscoveryProxy() {
                            var internalMessagingQos =
                                    new joynr.messaging.MessagingQos(
                                            provisioning.internalMessagingQos);
                            return joynr.proxyBuilder.build(DiscoveryProxy, {
                                domain : "io.joynr",
                                messagingQos : internalMessagingQos,
                                discoveryQos : new DiscoveryQos({
                                    discoveryScope : DiscoveryScope.LOCAL_ONLY
                                })
                            });
                        }

                        function createRoutingProxy() {
                            var internalMessagingQos =
                                    new joynr.messaging.MessagingQos(
                                            provisioning.internalMessagingQos);
                            return joynr.proxyBuilder.build(RoutingProxy, {
                                domain : "io.joynr",
                                messagingQos : internalMessagingQos,
                                discoveryQos : new DiscoveryQos({
                                    discoveryScope : DiscoveryScope.LOCAL_ONLY
                                })
                            });
                        }

                        function removeCapabilities(discoveryProxy, domain, interfaceName) {
                            var removedEntries = 0, successfullyRemovedFunction;

                            return new Promise(
                                    function(resolve, reject) {
                                        successfullyRemovedFunction = function(discoveredStubs) {
                                            removedEntries++;
                                            if (removedEntries === discoveredStubs.length) {
                                                resolve();
                                            }
                                        };

                                        discoveryProxy
                                                .lookup(
                                                        {
                                                            domain : domain,
                                                            interfaceName : interfaceName,
                                                            discoveryQos : new DiscoveryQos(
                                                                    {
                                                                        discoveryScope : DiscoveryScope.LOCAL_AND_GLOBAL
                                                                    })
                                                        })
                                                .then(
                                                        function(opArgs) {
                                                            var discoveredStubs = opArgs.result;
                                                            var discoveryId;
                                                            if (discoveredStubs.length === 0) {
                                                                resolve();
                                                                return;
                                                            }
                                                            for (discoveryId in discoveredStubs) {
                                                                if (discoveredStubs
                                                                        .hasOwnProperty(discoveryId)) {
                                                                    discoveryProxy
                                                                            .remove(
                                                                                    {
                                                                                        participantId : discoveredStubs[discoveryId].participantId
                                                                                    })
                                                                            .then(
                                                                                    successfullyRemovedFunction(discoveredStubs),
                                                                                    reject);
                                                                }
                                                            }
                                                        }).catch(function(error) {
                                                            expect(" error: " + error.message)
                                                                    .toBeFalsy();
                                                            reject(error);
                                                        });
                                    });
                        }

                        it(
                                "Create Radio Provider and register properly",
                                function() {
                                    var domain = "intertab-test-" + Date.now();
                                    /*
                                     * This test
                                     * - assumes a running cluster controller in a separate web worker
                                     *   (triggered in beforeEach)
                                     * - instantiates and registers a provider within the main frame
                                     *   for the test interface successfully, incl. communication
                                     *   with the cluster controller
                                     * - resolves a proxy within the main frame for the test
                                     *   interface successully, and successfully invokes methods on
                                     *   the proxy
                                     * - unregisters provider within the main frame for the test
                                     *   interface
                                     * - tries to resolve a proxy for the test interface, and fails
                                     *   (due to missing provider)
                                     */
                                    var isOn = true;
                                    var enumAttribute = Country.GERMANY;
                                    var enumArrayAttribute = [Country.GERMANY];
                                    var byteBufferAttribute = null;
                                    var stringMapAttribute = null;
                                    var attrProvidedImpl;
                                    var mixedSubscriptions = null;
                                    var numberOfStations = 0;
                                    var failingSyncAttribute = 0;
                                    var failingAsyncAttribute = 0;
                                    var typeDefForStruct = null;
                                    var typeDefForPrimitive = null;

                                    var providerRegistered, providerUnRegistered, proxyResolved;

                                    var messagingQos = new joynr.messaging.MessagingQos({
                                        ttl : 500
                                    });

                                    // the provider Quality of Service Parameters for registering
                                   // the Provider
                                    var providerQos = new joynr.types.ProviderQos({
                                        customParameters : [],
                                        providerVersion : 1,
                                        priority : Date.now(),
                                        scope : joynr.types.ProviderScope.GLOBAL,
                                        onChangeSubscriptions : true
                                    });

                                    // create radio provider
                                    radioProvider = joynr.providerBuilder.build(RadioProvider);

                                    // register attribute functions
                                    radioProvider.numberOfStations.registerGetter(function() {
                                        return numberOfStations;
                                    });

                                    radioProvider.numberOfStations.registerSetter(function(value) {
                                        numberOfStations = value;
                                    });

                                    radioProvider.attrProvidedImpl.registerGetter(function() {
                                        return attrProvidedImpl;
                                    });

                                    radioProvider.attrProvidedImpl.registerSetter(function(value) {
                                        attrProvidedImpl = value;
                                    });

                                    radioProvider.attributeTestingProviderInterface.registerGetter(function() {
                                       return undefined;
                                    });
 
                                    radioProvider.mixedSubscriptions.registerGetter(function() {
                                        return "interval";
                                    });

                                    radioProvider.mixedSubscriptions
                                            .registerSetter(function(value) {
                                                mixedSubscriptions = value;
                                            });

                                    radioProvider.isOn.registerGetter(function() {
                                        return isOn;
                                    });
                                    radioProvider.isOn.registerSetter(function(value) {
                                        isOn = value;
                                    });

                                    radioProvider.enumAttribute.registerGetter(function() {
                                        return enumAttribute;
                                    });

                                    radioProvider.enumAttribute.registerSetter(function(value) {
                                        enumAttribute = value;
                                    });

                                    radioProvider.enumArrayAttribute.registerGetter(function() {
                                        return enumArrayAttribute;
                                    });

                                    radioProvider.enumArrayAttribute.registerSetter(function(value) {
                                        enumArrayAttribute = value;
                                    });

                                    radioProvider.byteBufferAttribute.registerSetter(function(value) {
                                        byteBufferAttribute = value;
                                    });

                                    radioProvider.byteBufferAttribute.registerGetter(function(value) {
                                        return byteBufferAttribute;
                                    });

                                    radioProvider.stringMapAttribute.registerSetter(function(value) {
                                        stringMapAttribute = value;
                                    });

                                    radioProvider.stringMapAttribute.registerGetter(function(value) {
                                        return stringMapAttribute;
                                    });

                                    radioProvider.failingSyncAttribute.registerGetter(function() {
                                        return failingSyncAttribute;
                                    });

                                    radioProvider.failingAsyncAttribute.registerGetter(function() {
                                        return failingAsyncAttribute;
                                    });

                                    radioProvider.typeDefForStruct.registerSetter(function(value) {
                                        typeDefForStruct = value;
                                    });

                                    radioProvider.typeDefForStruct.registerGetter(function(value) {
                                        return typeDefForStruct;
                                    });

                                    radioProvider.typeDefForPrimitive.registerSetter(function(value) {
                                        typeDefForPrimitive = value;
                                    });

                                    radioProvider.typeDefForPrimitive.registerGetter(function(value) {
                                        return typeDefForPrimitive;
                                    });

                                    // register operation functions
                                    radioProvider.addFavoriteStation.registerOperation(function(
                                            opArgs) {
                                        return {
                                            returnValue : true
                                        };
                                    });

                                    // register operation function "operationWithEnumsAsInputAndOutput"
                                    radioProvider.operationWithEnumsAsInputAndOutput.registerOperation(function(opArgs) {
                                        /* the dummy implemenation returns the first element of the enumArrayInput.
                                         * If the input array is empty, it returns the enumInput
                                         */
                                        var returnValue = opArgs.enumInput;
                                        if (opArgs.enumArrayInput.length !== 0) {
                                            returnValue = opArgs.enumArrayInput[0];
                                        }
                                        return {
                                            enumOutput : returnValue
                                        };
                                    });

                                    // register operation function "operationWithMultipleOutputParameters"
                                    radioProvider.operationWithMultipleOutputParameters.registerOperation(function(opArgs) {
                                        var returnValue = {
                                            enumArrayOutput: opArgs.enumArrayInput,
                                            enumOutput: opArgs.enumInput,
                                            stringOutput: opArgs.stringInput,
                                            booleanOutput: opArgs.syncTest,
                                        };
                                        if (opArgs.syncTest) {
                                            return returnValue;
                                        }
                                        return new Promise(function(resolve, reject){
                                            resolve(returnValue);
                                        });
                                    });

                                    // register operation function "operationWithEnumsAsInputAndEnumArrayAsOutput"
                                    radioProvider.operationWithEnumsAsInputAndEnumArrayAsOutput.registerOperation(function(opArgs) {
                                        /* the dummy implemenation returns the first element of the enumArrayInput.
                                         * If the input array is empty, it returns the enumInput
                                         */
                                        var returnValue = opArgs.enumArrayInput;
                                        if (opArgs.enumInput !== undefined) {
                                            returnValue.push(opArgs.enumInput);
                                        }
                                        return {
                                            enumOutput : returnValue
                                        };
                                    });

                                    // register operation function "methodWithSingleArrayParameters"
                                    radioProvider.methodWithSingleArrayParameters.registerOperation(function(opArgs) {
                                        /* the dummy implementation transforms the incoming double values into
                                         * strings.
                                         */
                                        var stringArrayOut = [], element;
                                        if (opArgs.doubleArrayArg !== undefined) {
                                            for (element in opArgs.doubleArrayArg) {
                                                if (opArgs.doubleArrayArg.hasOwnProperty(element)) {
                                                    stringArrayOut.push(opArgs.doubleArrayArg[element].toString());
                                                }
                                            }
                                        }
                                        return {
                                            stringArrayOut: stringArrayOut
                                        };
                                    });

                                    radioProvider.methodWithByteBuffer.registerOperation(function(opArgs) {
                                        /* the dummy implementation returns the incoming byteBuffer
                                         */

                                        return {
                                            result: opArgs.input
                                        };
                                    });

                                    // register operation function "methodWithTypeDef"
                                    radioProvider.methodWithTypeDef.registerOperation(function(opArgs) {
                                        /* the dummy implementation returns the incoming data
                                         */

                                        return {
                                            typeDefStructOutput: opArgs.typeDefStructInput,
                                            typeDefPrimitiveOutput: opArgs.typeDefPrimitiveInput
                                        };
                                    });

                                    radioProvider.methodProvidedImpl.registerOperation(function(
                                            opArgs) {
                                        return {
                                            returnValue : opArgs.arg + "response"
                                        };
                                    });

                                    radioProvider.triggerBroadcasts.registerOperation(function(opArgs) {
                                        var i, outputParams, broadcast;
                                        if (opArgs.broadcastName === "broadcastWithEnum") {
                                            //broadcastWithEnum
                                            broadcast = radioProvider.broadcastWithEnum;
                                            outputParams = broadcast.createBroadcastOutputParameters();
                                            outputParams.setEnumOutput(Country.CANADA);
                                            outputParams.setEnumArrayOutput([Country.GERMANY, Country.ITALY]);
                                        } else if (opArgs.broadcastName === "weakSignal"){
                                            //weakSignal
                                            broadcast = radioProvider.weakSignal;
                                            outputParams = broadcast.createBroadcastOutputParameters();
                                            outputParams.setRadioStation("radioStation");
                                            outputParams.setByteBuffer([0,1,2,3,4,5,6,7,8,9,8,7,6,5,4,3,2,1,0]);
                                        } else if (opArgs.broadcastName === "broadcastWithTypeDefs"){
                                            //broadcastWithTypeDefs
                                            broadcast = radioProvider.broadcastWithTypeDefs;
                                            outputParams = broadcast.createBroadcastOutputParameters();
                                            outputParams.setTypeDefStructOutput(new RadioStation({
                                                name: "TestEnd2EndCommProviderWorker.broadcastWithTypeDefs.RadioStation",
                                                byteBuffer: []
                                            }));
                                            outputParams.setTypeDefPrimitiveOutput(123456);
                                        }
                                        for (i = 0; i < opArgs.times; i++) {
                                            broadcast.fire(outputParams);
                                        }
                                    });

                                    runs(function() {
                                        createDiscoveryProxy()
                                                .then(
                                                        function(newDiscoveryProxy) {
                                                            removeCapabilities(
                                                                    newDiscoveryProxy,
                                                                    domain,
                                                                    radioProvider.interfaceName)
                                                                    .then(
                                                                            function() {
                                                                                // register radio provider
                                                                                joynr.registration
                                                                                        .registerProvider(
                                                                                                domain,
                                                                                                radioProvider,
                                                                                                providerQos)
                                                                                        .then(
                                                                                                function() {

                                                                                                    providerRegistered =
                                                                                                            true;
                                                                                                    // publishing changed value regularly
                                                                                                    setInterval(
                                                                                                            function() {
                                                                                                                numberOfStations =
                                                                                                                        Math
                                                                                                                                .round(Math
                                                                                                                                        .random() * 256);
                                                                                                                radioProvider.numberOfStations
                                                                                                                        .valueChanged(numberOfStations);
                                                                                                            },
                                                                                                            2000);
                                                                                                }).catch(function(
                                                                                                        error) {
                                                                                                    providerRegistered =
                                                                                                            false;
                                                                                                    expect(
                                                                                                            " error: "
                                                                                                                + error.message)
                                                                                                            .toBeFalsy();
                                                                                                });
                                                                            });
                                                        }).catch(function(error) {
                                                            expect(" error: " + error.message)
                                                                    .toBeFalsy();
                                                        });
                                    });

                                    waitsFor(function() {
                                        return providerRegistered;
                                    }, "Provider to be registered successfully", provisioning.ttl);

                                    runs(function() {
                                        expect(providerRegistered).toBeTruthy();
                                        joynr.proxyBuilder
                                                .build(RadioProxy, {
                                                    domain : domain,
                                                    messagingQos : messagingQos
                                                })
                                                .then(
                                                        function(newRadioProxy) {
                                                            expect(newRadioProxy).toBeDefined();
                                                            expect(
                                                                    newRadioProxy.providerParticipantId)
                                                                    .toEqual(
                                                                            joynr.participantIdStorage
                                                                                    .getParticipantId(
                                                                                            domain,
                                                                                            radioProvider));
                                                            radioProxy = newRadioProxy;
                                                            proxyResolved = true;
                                                        }).catch(function(error) {
                                                            proxyResolved = false;
                                                            expect(" error: " + error.message)
                                                                    .toBeFalsy();
                                                        });
                                    });

                                    waitsFor(function() {
                                        return radioProxy !== undefined;
                                    }, "Proxy to be resolved successfully", provisioning.ttl);

                                    runs(function() {
                                        expect(proxyResolved).toBeTruthy();
                                        proxyResolved = undefined;
                                        radioProxy.addFavoriteStation({
                                            radioStation : "radioStation"
                                        }).then(
                                                function(opArgs) {
                                                    var result = opArgs.returnValue;
                                                    expect(result).toEqual(true);
                                                    joynr.registration.unregisterProvider(
                                                            domain,
                                                            radioProvider).then(function() {
                                                        providerUnRegistered = true;
                                                    }).catch(function(error) {
                                                        providerUnRegistered = false;
                                                    });
                                                }).catch(function(error) {
                                                    expect(" error: " + error.message).toBeFalsy();
                                                });
                                    });

                                    waitsFor(function() {
                                        return providerUnRegistered;
                                    }, "Provider to be unregistered successfully", provisioning.ttl);

                                    runs(function() {
                                        expect(providerUnRegistered).toBeTruthy();
                                        joynr.proxyBuilder
                                                .build(RadioProxy, {
                                                    domain : domain,
                                                    messagingQos : messagingQos,
                                                    discoveryQos : {
                                                        discoveryTimeout : discoveryTimeout
                                                    }
                                                })
                                                .then(
                                                        function(newRadioProxy) {
                                                            proxyResolved = true;
                                                            expect(
                                                                    " error: proxy resolved, even if no provider shall be contained in the capability directory")
                                                                    .toBeFalsy();
                                                        }).catch(function(error) {
                                                            proxyResolved = false;
                                                        });
                                    });

                                    waitsFor(function() {
                                        return proxyResolved === false;
                                    }, "Proxy not be resolved", discoveryTimeout + 500);

                                    runs(function() {
                                        expect(proxyResolved).toBeFalsy();
                                    });

                                });

                        it(
                                "Create separate joynr instance with provider and arbitrate",
                                function() {
                                    var domain = "intertab-test-" + Date.now();
                                    /*
                                     * This test
                                     * - assumes a running cluster controller in a separate web
                                     *   worker (triggered in beforeEach)
                                     * - creates a second web worker, where a separate joynr
                                     *   instance is running (incl. libjoynr and cc)
                                     * - the second web worker registers a provider for the test
                                     *   interface
                                     * - resolves a proxy within the main frame for the test
                                     *   interface successully, by resolveing the provider in the
                                     *   separate web worker
                                     * - test successfully invokes methods on the proxy
                                     */
                                    var proxyResolved, radioProxy, providerWorkerId, testFinished;

                                    runs(function() {
                                        IntegrationUtils.initializeWebWorker(
                                                "TestEnd2EndCommProviderWorker",
                                                provisioningSuffix,
                                                domain).then(function(newWorkerId) {
                                            providerWorkerId = newWorkerId;
                                        });
                                    });
                                    waitsFor(function() {
                                        return providerWorkerId !== undefined;
                                    }, "WebWorker to be ready", provisioning.ttl);

                                    runs(function() {
                                        joynr.proxyBuilder.build(RadioProxy, {
                                            domain : domain,
                                            messagingQos : messagingQos
                                        }).then(function(newRadioProxy) {
                                            expect(newRadioProxy).toBeDefined();
                                            radioProxy = newRadioProxy;
                                            proxyResolved = true;
                                        }).catch(function(error) {
                                            proxyResolved = false;
                                            expect(" error: " + error.message).toBeFalsy();
                                        });
                                    });

                                    waitsFor(function() {
                                        return radioProxy !== undefined;
                                    }, "Proxy to be resolved successfully", provisioning.ttl);

                                    runs(function() {
                                        expect(proxyResolved).toBeTruthy();
                                        proxyResolved = undefined;
                                        radioProxy.addFavoriteStation({
                                            radioStation : "radioStation"
                                        }).then(
                                                function(opArgs) {
                                                    var result = opArgs.returnValue;
                                                    expect(result).toEqual(false);
                                                    IntegrationUtils.shutdownWebWorker(
                                                            providerWorkerId).then(function() {
                                                        testFinished = true;
                                                    });
                                                }).catch(function(error) {
                                                    IntegrationUtils
                                                            .shutdownWebWorker(providerWorkerId);
                                                    expect(" error: " + error.message).toBeFalsy();
                                                });
                                    });

                                    waitsFor(function() {
                                        return testFinished !== undefined;
                                    }, "test to be finished", provisioning.ttl);
                                });

                        it(
                                "Create discovery proxy and check if the cc discovery provider works properly",
                                function() {
                                    var domain = "intertab-test-" + Date.now(), discoveredEntries;
                                    /*
                                     * This test
                                     * - assumes a running cluster controller in a separate web
                                     *   worker (triggered in beforeEach)
                                     * - creates a second web worker, where a separate joynr
                                     *   instance is running (incl. libjoynr and cc)
                                     * - the second web worker registers a provider for the test
                                     *   interface
                                     * - resolves a discovery proxy within the main frame for the
                                     *   test interface successully, by resolving the respective
                                     *   provider in the cc web worker
                                     * - expects the test provider to be registered, by invoking the
                                     *   lookup method of the discovery interface
                                     * - unregisters the provider, and the next lookup shall provide
                                     *   an empty list
                                     * - performs some separate add/remove/lookup calls on the
                                     *   routing interface, to see if the routing provider behaves
                                     *   as expected
                                     */
                                    var proxyResolved, discoveryProxy, providerWorkerId, testFinished;

                                    runs(function() {
                                        IntegrationUtils.initializeWebWorker(
                                                "TestEnd2EndCommProviderWorker",
                                                provisioningSuffix,
                                                domain).then(
                                                function(newWorkerId) {
                                                    providerWorkerId = newWorkerId;
                                                    createDiscoveryProxy().then(
                                                            function(newDiscoveryProxy) {
                                                                discoveryProxy = newDiscoveryProxy;
                                                                proxyResolved = true;
                                                            }).catch(function(error) {
                                                                proxyResolved = false;
                                                            });
                                                });
                                    });

                                    waitsFor(
                                            function() {
                                                return proxyResolved !== undefined;
                                            },
                                            "Discovery proxy to be resolved successfully",
                                            provisioning.ttl);

                                    runs(function() {
                                        expect(proxyResolved).toBeTruthy();
                                        proxyResolved = undefined;
                                        discoveryProxy
                                                .lookup(
                                                        {
                                                            domain : domain,
                                                            interfaceName : "vehicle/Radio",
                                                            discoveryQos : new DiscoveryQos(
                                                                    {
                                                                        discoveryScope : DiscoveryScope.LOCAL_THEN_GLOBAL
                                                                    })
                                                        })
                                                .then(
                                                        function(opArgs) {
                                                            var discoveredStubs = opArgs.result;
                                                            expect(discoveredStubs.length).toEqual(
                                                                    1);
                                                            IntegrationUtils
                                                                    .shutdownWebWorker(
                                                                            providerWorkerId)
                                                                    .then(
                                                                            function() {
                                                                                discoveryProxy
                                                                                        .lookup(
                                                                                                {
                                                                                                    domain : domain,
                                                                                                    interfaceName : "vehicle/Radio",
                                                                                                    discoveryQos : new DiscoveryQos(
                                                                                                            {
                                                                                                                discoveryScope : DiscoveryScope.LOCAL_THEN_GLOBAL
                                                                                                            })
                                                                                                })
                                                                                        .then(
                                                                                                function(
                                                                                                        opArgs) {
                                                                                                    var discoveredStubs = opArgs.result;
                                                                                                    discoveredEntries =
                                                                                                            discoveredStubs;
                                                                                                    expect(
                                                                                                            discoveredStubs.length)
                                                                                                            .toEqual(
                                                                                                                    0);
                                                                                                }).catch(function(
                                                                                                        error) {
                                                                                                    expect(
                                                                                                            " error: "
                                                                                                                + error.message)
                                                                                                            .toBeFalsy();
                                                                                                });
                                                                            }).catch(function(error) {
                                                                                expect(
                                                                                        " error: "
                                                                                            + error.message)
                                                                                        .toBeFalsy();
                                                                            });
                                                        });
                                    });

                                    waitsFor(
                                            function() {
                                                return discoveredEntries !== undefined;
                                            },
                                            "successfull discovery lookup for the radio interface",
                                            provisioning.ttl);

                                    runs(function() {
                                        var participantId = "222", participantId2 = "2222", interfaceName =
                                                "TestDiscoverAPI", providerQos;
                                        var errorFct = function(error) {
                                            expect(" error: " + error.message).toBeFalsy();
                                        };

                                        providerQos = new joynr.types.ProviderQos({
                                            customParameters : [],
                                            providerVersion : 1,
                                            priority : Date.now(),
                                            scope : joynr.types.ProviderScope.GLOBAL,
                                            onChangeSubscriptions : true
                                        });

                                        discoveryProxy
                                                .add({
                                                    discoveryEntry : new DiscoveryEntry({
                                                        domain : domain,
                                                        interfaceName : interfaceName,
                                                        participantId : participantId,
                                                        qos : providerQos,
                                                        connections : []
                                                    })
                                                })
                                                .then(
                                                        function() {
                                                            discoveryProxy
                                                                    .lookup(
                                                                            {
                                                                                domain : domain,
                                                                                interfaceName : interfaceName,
                                                                                discoveryQos : new DiscoveryQos(
                                                                                        {
                                                                                            discoveryScope : DiscoveryScope.LOCAL_ONLY
                                                                                        })
                                                                            })
                                                                    .then(
                                                                            function(
                                                                                    opArgs) {
                                                                                var discoveredStubs = opArgs.result;
                                                                                expect(
                                                                                        discoveredStubs.length)
                                                                                        .toEqual(1);
                                                                                expect(
                                                                                        discoveredStubs[0].participantId)
                                                                                        .toEqual(
                                                                                                participantId);
                                                                                discoveryProxy
                                                                                        .add(
                                                                                                {
                                                                                                    discoveryEntry : new DiscoveryEntry(
                                                                                                            {
                                                                                                                domain : domain,
                                                                                                                interfaceName : interfaceName,
                                                                                                                participantId : participantId2,
                                                                                                                qos : providerQos,
                                                                                                                connections : []
                                                                                                            })
                                                                                                })
                                                                                        .then(
                                                                                                function() {
                                                                                                    discoveryProxy
                                                                                                            .lookup(
                                                                                                                    {
                                                                                                                        domain : domain,
                                                                                                                        interfaceName : interfaceName,
                                                                                                                        discoveryQos : new DiscoveryQos(
                                                                                                                                {
                                                                                                                                    discoveryScope : DiscoveryScope.LOCAL_ONLY
                                                                                                                                })
                                                                                                                    })
                                                                                                            .then(
                                                                                                                    function(
                                                                                                                            opArgs) {
                                                                                                                        var discoveredStubs = opArgs.result;
                                                                                                                        expect(
                                                                                                                                discoveredStubs.length)
                                                                                                                                .toEqual(
                                                                                                                                        2);
                                                                                                                        expect(
                                                                                                                                discoveredStubs[0].participantId)
                                                                                                                                .toEqual(
                                                                                                                                        participantId2);
                                                                                                                        expect(
                                                                                                                                discoveredStubs[1].participantId)
                                                                                                                                .toEqual(
                                                                                                                                        participantId);
                                                                                                                        discoveryProxy
                                                                                                                                .remove(
                                                                                                                                        {
                                                                                                                                            participantId : participantId
                                                                                                                                        })
                                                                                                                                .then(
                                                                                                                                        function() {
                                                                                                                                            discoveryProxy
                                                                                                                                                    .remove(
                                                                                                                                                            {
                                                                                                                                                                participantId : participantId2
                                                                                                                                                            })
                                                                                                                                                    .then(
                                                                                                                                                            function() {
                                                                                                                                                                discoveryProxy
                                                                                                                                                                        .lookup(
                                                                                                                                                                                {
                                                                                                                                                                                    domain : domain,
                                                                                                                                                                                    interfaceName : interfaceName,
                                                                                                                                                                                    discoveryQos : new DiscoveryQos(
                                                                                                                                                                                            {
                                                                                                                                                                                                discoveryScope : DiscoveryScope.LOCAL_ONLY
                                                                                                                                                                                            })
                                                                                                                                                                                })
                                                                                                                                                                        .then(
                                                                                                                                                                                function(
                                                                                                                                                                                        opArgs) {
                                                                                                                                                                                    var discoveredStubs = opArgs.result;
                                                                                                                                                                                    expect(
                                                                                                                                                                                            discoveredStubs.length)
                                                                                                                                                                                            .toEqual(
                                                                                                                                                                                                    0);
                                                                                                                                                                                    testFinished =
                                                                                                                                                                                            true;
                                                                                                                                                                                }).catch(errorFct);
                                                                                                                                                            }).catch(errorFct);
                                                                                                                                        }).catch(errorFct);
                                                                                                                    }).catch(errorFct);
                                                                                                }).catch(errorFct);
                                                                            }).catch(errorFct);
                                                        }).catch(errorFct);

                                    });

                                    waitsFor(function() {
                                        return testFinished !== undefined;
                                    }, "finishing test", provisioning.ttl);

                                });

                        it(
                                "Create routing proxy and check if the cc routing provider works properly",
                                function() {
                                    var domain = "intertab-test-" + Date.now(), discoveredEntries;
                                    /*
                                     * This test
                                     * - assumes a running cluster controller in a separate web
                                     *   worker (triggered in beforeEach)
                                     * - creates a second web worker, where a separate joynr
                                     *   instance is running (incl. libjoynr and cc)
                                     * - the second web worker registers a provider for the test
                                     *   interface
                                     * - resolves a routing proxy within the main frame for the test
                                     *   interface successully, by resolving the respective provider
                                     *   in the cc web worker
                                     * - in a first step, the routing proxy tries to resolve the
                                     *   provider of the test interface and fails, because no routing
                                     *   has been created
                                     *   as long as no proxy has been resolved for the test interface
                                     * - then a proxy is resolved for the test interface
                                     * - now, the routing provider shall know the hop for the test
                                     *   interface provider
                                     */
                                    var testParticipantId = "testparticipant-" + Date.now();
                                    var proxyResolved;
                                    var radioProxy;
                                    var routingProxy;
                                    var providerWorkerId;
                                    var providerParticipantId;
                                    var resolveSucceed;
                                    var channelParticipantResolved;
                                    var browserParticipantResolved;
                                    var webSocketParticipantResolved;
                                    var commonApiDbusParticipantResolved;

                                    var errorFct = function(error) {
                                        expect(" error: " + error.message).toBeFalsy();
                                    };

                                    runs(function() {
                                        IntegrationUtils
                                                .initializeWebWorker(
                                                        "TestEnd2EndCommProviderWorker",
                                                        provisioningSuffix,
                                                        domain)
                                                .then(
                                                        function(newWorkerId) {
                                                            providerWorkerId = newWorkerId;
                                                            IntegrationUtils
                                                                    .startWebWorker(newWorkerId)
                                                                    .then(
                                                                            function(
                                                                                    newProviderParticipantId) {
                                                                                providerParticipantId =
                                                                                        newProviderParticipantId;
                                                                                createRoutingProxy()
                                                                                        .then(
                                                                                                function(
                                                                                                        newRoutingProxy) {
                                                                                                    proxyResolved =
                                                                                                            true;
                                                                                                    routingProxy =
                                                                                                            newRoutingProxy;
                                                                                                }).catch(function(
                                                                                                        error) {
                                                                                                    proxyResolved =
                                                                                                            false;
                                                                                                });
                                                                            }).catch(function(error) {
                                                                                proxyResolved =
                                                                                        false;
                                                                            });
                                                        });
                                    });

                                    waitsFor(
                                            function() {
                                                return routingProxy !== undefined;
                                            },
                                            "Routing proxy to be resolved successfully",
                                            provisioning.ttl);

                                    runs(function() {
                                        expect(proxyResolved).toBeTruthy();
                                        proxyResolved = undefined;
                                        routingProxy
                                                .resolveNextHop({
                                                    participantId : providerParticipantId
                                                })
                                                .then(
                                                        function(opArgs) {
                                                            var success = opArgs.resolved;
                                                            expect(success).toBeFalsy();
                                                            joynr.proxyBuilder
                                                                    .build(RadioProxy, {
                                                                        domain : domain,
                                                                        messagingQos : messagingQos
                                                                    })
                                                                    .then(
                                                                            function(newRadioProxy) {
                                                                                expect(
                                                                                        newRadioProxy)
                                                                                        .toBeDefined();
                                                                                radioProxy =
                                                                                        newRadioProxy;
                                                                                proxyResolved =
                                                                                        true;
                                                                            }).catch(function(error) {
                                                                                proxyResolved =
                                                                                        false;
                                                                                IntegrationUtils
                                                                                        .shutdownWebWorker(providerWorkerId);
                                                                                expect(
                                                                                        " error: "
                                                                                            + error.message)
                                                                                        .toBeFalsy();
                                                                            });
                                                        }).catch(function(error) {
                                                            IntegrationUtils
                                                                    .shutdownWebWorker(providerWorkerId);
                                                            expect("error: " + error).toBeFalsy();
                                                        });
                                    });

                                    waitsFor(function() {
                                        return radioProxy !== undefined;
                                    }, "Radio proxy to be resolve successfully", provisioning.ttl);

                                    runs(function() {
                                        routingProxy.resolveNextHop({
                                            participantId : providerParticipantId
                                        }).then(
                                                function(opArgs) {
                                                    var success = opArgs.resolved;
                                                    expect(success).toBeTruthy();
                                                    IntegrationUtils.shutdownWebWorker(
                                                            providerWorkerId).then(function() {
                                                        resolveSucceed = success;
                                                    });
                                                }).catch(function(error) {
                                                    IntegrationUtils
                                                            .shutdownWebWorker(providerWorkerId);
                                                });
                                    });

                                    waitsFor(function() {
                                        return resolveSucceed;
                                    }, "Routing provider to be succeed", provisioning.ttl);

                                    runs(function() {
                                        var channelAddress = new ChannelAddress({
                                            channelId : "channelId"
                                        });

                                        routingProxy.resolveNextHop({
                                            participantId : testParticipantId
                                        }).then(function(opArgs) {
                                            var success = opArgs.resolved;
                                            expect(success).toBeFalsy();
                                            routingProxy.addNextHop({
                                                participantId : testParticipantId,
                                                channelAddress : channelAddress
                                            }).then(function() {
                                                routingProxy.resolveNextHop({
                                                    participantId : testParticipantId
                                                }).then(function(opArgs) {
                                                    var success = opArgs.resolved;
                                                    expect(success).toBeTruthy();
                                                    routingProxy.removeNextHop({
                                                        participantId : testParticipantId
                                                    }).then(function() {
                                                        channelParticipantResolved = true;
                                                    }).catch(errorFct);
                                                }).catch(errorFct);
                                            }).catch(errorFct);
                                        }).catch(errorFct);
                                    });

                                    waitsFor(
                                            function() {
                                                return channelParticipantResolved;
                                            },
                                            "test participant as ChannelAddress to be resolved ",
                                            provisioning.ttl);

                                    runs(function() {
                                        var browserAddress = new BrowserAddress({
                                            windowId : "windowId"
                                        });
                                        routingProxy.resolveNextHop({
                                            participantId : testParticipantId
                                        }).then(function(opArgs) {
                                            var success = opArgs.resolved;
                                            expect(success).toBeFalsy();
                                            routingProxy.addNextHop({
                                                participantId : testParticipantId,
                                                browserAddress : browserAddress
                                            }).then(function() {
                                                routingProxy.resolveNextHop({
                                                    participantId : testParticipantId
                                                }).then(function(opArgs) {
                                                    var success = opArgs.resolved;
                                                    expect(success).toBeTruthy();
                                                    routingProxy.removeNextHop({
                                                        participantId : testParticipantId
                                                    }).then(function() {
                                                        browserParticipantResolved = true;
                                                    }).catch(errorFct);
                                                }).catch(errorFct);
                                            }).catch(errorFct);
                                        }).catch(errorFct);
                                    });

                                    waitsFor(
                                            function() {
                                                return browserParticipantResolved;
                                            },
                                            "test participant as BrowserAddress to be resolved ",
                                            provisioning.ttl);

                                    runs(function() {
                                        var webSocketAddress = new WebSocketAddress({
                                            port : 66,
                                            host : "host"
                                        });

                                        routingProxy.resolveNextHop({
                                            participantId : testParticipantId
                                        }).then(function(opArgs) {
                                            var success = opArgs.resolved;
                                            expect(success).toBeFalsy();
                                            routingProxy.addNextHop({
                                                participantId : testParticipantId,
                                                webSocketAddress : webSocketAddress
                                            }).then(function() {
                                                routingProxy.resolveNextHop({
                                                    participantId : testParticipantId
                                                }).then(function(opArgs) {
                                                    var success = opArgs.resolved;
                                                    expect(success).toBeTruthy();
                                                    routingProxy.removeNextHop({
                                                        participantId : testParticipantId
                                                    }).then(function() {
                                                        webSocketParticipantResolved = true;
                                                    }).catch(errorFct);
                                                }).catch(errorFct);
                                            }).catch(errorFct);
                                        }).catch(errorFct);
                                    });

                                    waitsFor(
                                            function() {
                                                return webSocketParticipantResolved;
                                            },
                                            "test participant as WebSocketAddress to be resolved ",
                                            provisioning.ttl);

                                    runs(function() {
                                        var commonApiDbusAddress = new CommonApiDbusAddress({
                                            domain : "domain",
                                            participantId : "participantId",
                                            serviceName : "serviceName"
                                        });

                                        routingProxy
                                                .resolveNextHop({
                                                    participantId : testParticipantId
                                                })
                                                .then(
                                                        function(opArgs) {
                                                            var success = opArgs.resolved;
                                                            expect(success).toBeFalsy();
                                                            routingProxy
                                                                    .addNextHop(
                                                                            {
                                                                                participantId : testParticipantId,
                                                                                commonApiDbusAddress : commonApiDbusAddress
                                                                            })
                                                                    .then(
                                                                            function() {
                                                                                routingProxy
                                                                                        .resolveNextHop(
                                                                                                {
                                                                                                    participantId : testParticipantId
                                                                                                })
                                                                                        .then(
                                                                                                function(
                                                                                                        opArgs) {
                                                                                                    var success = opArgs.resolved;
                                                                                                    expect(
                                                                                                            success)
                                                                                                            .toBeTruthy();
                                                                                                    routingProxy
                                                                                                            .removeNextHop(
                                                                                                                    {
                                                                                                                        participantId : testParticipantId
                                                                                                                    })
                                                                                                            .then(
                                                                                                                    function() {
                                                                                                                        routingProxy
                                                                                                                                .resolveNextHop(
                                                                                                                                        {
                                                                                                                                            participantId : testParticipantId
                                                                                                                                        })
                                                                                                                                .then(
                                                                                                                                        function(
                                                                                                                                                opArgs) {
                                                                                                                                            var success = opArgs.resolved;
                                                                                                                                            expect(
                                                                                                                                                    success)
                                                                                                                                                    .toBeFalsy();
                                                                                                                                            commonApiDbusParticipantResolved =
                                                                                                                                                    true;
                                                                                                                                        }).catch(errorFct);
                                                                                                                    }).catch(errorFct);
                                                                                                }).catch(errorFct);
                                                                            }).catch(errorFct);
                                                        }).catch(errorFct);
                                    });

                                    waitsFor(
                                            function() {
                                                return commonApiDbusParticipantResolved;
                                            },
                                            "test participant as CommonApiDbusAddress to be resolved ",
                                            provisioning.ttl);

                                });
                    });
        });
