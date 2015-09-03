/*jslint es5: true */
/*global Promise: true, WorkerUtils: true, importScripts: true, joynr: true, RadioProvider: true, domain: true, interfaceNameComm: true, providerParticipantIdComm: true, providerChannelIdComm: true, globalCapDirCapability: true, channelUrlDirCapability: true */

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

// anything that you load here is served through the jsTestDriverServer, if you add an entry you
// have to make it available through the jsTestDriverIntegrationTests.conf
importScripts("WorkerUtils.js");
importScripts("../joynr/provisioning/provisioning_root.js");
importScripts("LocalStorageSimulator.js");

importScripts("../../classes/js/joynr.js");
importScripts("../joynr/provisioning/provisioning_common.js");
importScripts("../joynr/provisioning/provisioning_cc.js");
importScripts("provisioning_end2end_common.js");
importScripts("../joynr/vehicle/RadioProvider.js");
importScripts("../joynr/vehicle/radiotypes/RadioStation.js");
importScripts("../../classes/lib/bluebird.js");

var Promise = Promise.Promise;
// attribute value for provider
var isOn = true;
var attrProvidedImpl;
var numberOfStations = -1;
var mixedSubscriptions = null;

var providerDomain;
var libjoynrAsync;

// this delay tries to correct the initialization delay between the web worker and the test driver
var valueChangedInterval = 500;
var mixedSubscriptionDelay = 1500;

var radioProvider;

var providerQos;

function initializeTest(provisioningSuffix, providedDomain) {
    return new Promise(function(resolve, reject) {

        // set joynr provisioning
        providerDomain = (providedDomain !== undefined ? providedDomain : domain);
        localStorage.setItem(
                "joynr.participants." + providerDomain + interfaceNameComm,
                providerParticipantIdComm + provisioningSuffix);
        joynr.provisioning.persistency = "localStorage";

        joynr.provisioning.channelId = providerChannelIdComm + provisioningSuffix;
        joynr.provisioning.logging = {
            configuration : {
                name : "TestEnd2EndCommProviderWorkLogging",
                appenders : {
                    WebWorker : {
                        name : "WEBWORKER"
                    }
                },
                loggers : {
                    root : {
                        level : "debug",
                        AppenderRef : {
                            ref : "WEBWORKER"
                        }
                    }
                }
            }
        };

        joynr.load(joynr.provisioning, function(error, asynclib) {
            if (error) {
                throw error;
            }

            libjoynrAsync = asynclib;
            providerQos = new libjoynrAsync.types.ProviderQos({
                customParameters : [],
                providerVersion : 1,
                priority : Date.now(),
                scope : libjoynrAsync.types.ProviderScope.GLOBAL,
                onChangeSubscriptions : true
            });

            // build the provider
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

            radioProvider.mixedSubscriptions.registerGetter(function() {
                return "interval";
            });

            radioProvider.mixedSubscriptions.registerSetter(function(value) {
                mixedSubscriptions = value;
            });

            radioProvider.isOn.registerGetter(function() {
                return isOn;
            });
            radioProvider.isOn.registerSetter(function(value) {
                isOn = value;
            });

            // register operation functions
            radioProvider.addFavoriteStation.registerOperation(function(opArgs) {
                // retrieve radioStation name for both overloaded version
                var name = opArgs.radioStation.name || opArgs.radioStation;
                // returns true if radiostation name contains the string "true"
                return !!name.match(/true/);
            });

            radioProvider.methodProvidedImpl.registerOperation(function(opArgs) {
                return opArgs.arg;
            });

            providerQos.priority = Date.now();
            // register provider at the given providerDomain
            libjoynrAsync.capabilities.registerCapability(
                    "",
                    providerDomain,
                    radioProvider,
                    providerQos).then(function() {
                // signal test driver that we are ready
                resolve(libjoynrAsync);
            }).catch(function(error) {
                reject(error);
                throw new Error("error registering provider: " + error);
            });
        });
    });
}

function startTest() {
    return new Promise(function(resolve, reject) {
        // change attribute value of numberOfStations periodically
        libjoynrAsync.util.LongTimer.setInterval(function() {
            radioProvider.numberOfStations.valueChanged(++numberOfStations);
        }, valueChangedInterval);

        libjoynrAsync.util.LongTimer.setTimeout(function() {
            radioProvider.mixedSubscriptions.valueChanged("valueChanged1");
            libjoynrAsync.util.LongTimer.setTimeout(function() {
                radioProvider.mixedSubscriptions.valueChanged("valueChanged2");
            }, 10);
        }, mixedSubscriptionDelay);
        resolve(libjoynrAsync.participantIdStorage.getParticipantId(
                "",
                providerDomain,
                radioProvider));
    });
}
function terminateTest() {
    return libjoynrAsync.capabilities.unregisterCapability("", providerDomain, radioProvider);
}
