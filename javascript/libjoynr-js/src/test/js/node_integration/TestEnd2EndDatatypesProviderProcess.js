/* domain: true, interfaceNameDatatypes: true, globalCapDirCapability: true, channelUrlDirCapability: true */

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

var ChildProcessUtils = require("./ChildProcessUtils");
ChildProcessUtils.overrideRequirePaths();

var Promise = require("../../../main/js/global/Promise");
// anything that you load here is served through the jsTestDriverServer, if you add an entry you
// have to make it available through the jsTestDriverIntegrationTests.conf
var ChildProcessUtils = require("./ChildProcessUtils");
var joynr = require("joynr");
var provisioning = require("../../resources/joynr/provisioning/provisioning_cc.js");

var DatatypesProvider = require("../../generated/joynr/datatypes/DatatypesProvider.js");
var TestEnd2EndDatatypesTestData = require("./TestEnd2EndDatatypesTestData");

var providerDomain;

// attribute values for provider
var currentAttributeValue;

var datatypesProvider;

var providerQos;

function getObjectType(obj) {
    if (obj === null || obj === undefined) {
        throw new Error("cannot determine the type of an undefined object");
    }
    var funcNameRegex = /function ([$\w]+)\(/;
    var results = funcNameRegex.exec(obj.constructor.toString());
    return results && results.length > 1 ? results[1] : "";
}

function getter() {
    return currentAttributeValue;
}

function setter(value) {
    currentAttributeValue = value;
}

function initializeTest(provisioningSuffix, providedDomain) {
    providerDomain = providedDomain;
    provisioning.persistency = "localStorage";
    provisioning.channelId = "End2EndDatatypesTestParticipantId" + provisioningSuffix;
    provisioning.logging = {
        configuration: {
            appenders: {
                appender: [
                    {
                        type: "Console",
                        name: "STDOUT",
                        PatternLayout: {
                            pattern: "[%d{HH:mm:ss,SSS}][%c][%p] %m{2}"
                        }
                    }
                ]
            },
            loggers: {
                root: {
                    level: "debug",
                    AppenderRef: [
                        {
                            ref: "STDOUT"
                        }
                    ]
                }
            }
        }
    };

    joynr.selectRuntime("inprocess");
    return joynr
        .load(provisioning)
        .then(function(newJoynr) {
            providerQos = new joynr.types.ProviderQos({
                customParameters: [],
                priority: Date.now(),
                scope: joynr.types.ProviderScope.GLOBAL,
                supportsOnChangeSubscriptions: true
            });

            // build the provider
            datatypesProvider = joynr.providerBuilder.build(DatatypesProvider, {});

            var i;
            // there are so many attributes for testing different datatypes => register them
            // all by cycling over their names in the attribute
            for (i = 0; i < TestEnd2EndDatatypesTestData.length; ++i) {
                var attribute = datatypesProvider[TestEnd2EndDatatypesTestData[i].attribute];
                attribute.registerGetter(getter);
                attribute.registerSetter(setter);
            }

            // registering operation functions
            datatypesProvider.getJavascriptType.registerOperation(function(opArgs) {
                return {
                    javascriptType: getObjectType(opArgs.arg)
                };
            });
            datatypesProvider.getArgumentBack.registerOperation(function(opArgs) {
                return {
                    returnValue: opArgs.arg
                };
            });
            datatypesProvider.multipleArguments.registerOperation(function(opArgs) {
                return {
                    serialized: JSON.stringify(opArgs)
                };
            });

            // register provider at the given domain
            return newJoynr.registration
                .registerProvider(providerDomain, datatypesProvider, providerQos)
                .then(function() {
                    return Promise.resolve(newJoynr);
                });
        })
        .catch(function(error) {
            throw error;
        });
}

function startTest() {
    // nothing to do here, everything is already performed in initialize
    return Promise.resolve();
}

function terminateTest() {
    return joynr.registration.unregisterProvider(providerDomain, datatypesProvider);
}

ChildProcessUtils.registerHandlers(initializeTest, startTest, terminateTest);
