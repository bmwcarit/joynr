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

var requirejs = require("requirejs");

requirejs.onError =
        function(err) {
            // report the error as a failing jasmine test
            describe(
                    "requirejs",
                    function() {
                        it(
                                "global.errback",
                                function() {
                                    expect(
                                            "a "
                                                + err.requireType
                                                + " was thrown to the global requirejs error handler, required modules: "
                                                + err.requireModules.toString()).toBeFalsy();
                                });
                    });
            // overwrite default behavior of joynr: do not throw the error, instead just print it
            console.error(err);
        };

requirejs.config({
    nodeRequire : require,
    baseUrl : "${project.build.outputDirectory}",
    paths : {
        "JsonParser" : "lib/JsonParser",
        "uuid" : "lib/uuid-annotated",

        "joynr/system/LoggingManager" : "joynr/system/LoggingManagerNode",
        "joynr/security/PlatformSecurityManager" : "joynr/security/PlatformSecurityManagerNode",
        "global/LocalStorage" : "../test-classes/global/LocalStorageNodeTests",
        "atmosphere" : "lib/atmosphereNode",
        "log4javascript" : "lib/log4javascriptNode",
        "global/WebSocket" : "../test-classes/global/WebSocketMock",

        "joynr/Runtime" : "joynr/Runtime.inprocess",

        "joynr/tests" : "../test-classes/joynr/tests",
        "joynr/types" : "../classes/joynr/types",
        "joynr/types/TestTypes" : "../test-classes/joynr/types/TestTypes",
        "joynr/types/TestTypesWithoutVersion" : "../test-classes/joynr/types/TestTypesWithoutVersion",
        "joynr/vehicle" : "../test-classes/joynr/vehicle",
        "joynr/datatypes" : "../test-classes/joynr/datatypes",
        "joynr/provisioning" : "../test-classes/joynr/provisioning",
        "test/data" : "../test-classes/test/data",
        "Date" : "../test-classes/global/Date"
    }
});

var JoynrTestRequire = require("${project.build.directory}/test-classes/JoynrTestRequire.js");
global.joynrTestRequire = new JoynrTestRequire(requirejs).joynrTestRequire;
