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
var allTestFiles = [];
var TEST_REGEXP = /Test.js$/i;

Object.keys(window.__karma__.files).forEach(function(file) {
    if (TEST_REGEXP.test(file)) {
        var normalizedTestModule = file.replace(/^\/base\/|\.js$/g, '');
        allTestFiles.push('../' + normalizedTestModule);
    }
});

require.config({
    baseUrl: '/base/classes',

    paths : {
        "joynr/Runtime" : "joynr/Runtime.inprocess",
        "atmosphere" : "lib/atmosphere",
        "log4javascriptDependency" : "lib/log4javascript",
        "bluebird" : "lib/bluebird",
        "mqtt" : "lib/mqtt",
        "JsonParser" : "lib/JsonParser",
        "uuid" : "lib/uuid-annotated",
        "global/WebSocket": "../test-classes/global/WebSocketMock",
        "global/WaitsFor": "../test-classes/global/WaitsFor",
        "Date" : "../test-classes/global/Date",
        "joynr/datatypes": "../test-classes/joynr/datatypes",
        "joynr/tests": "../test-classes/joynr/tests",
        "joynr/types/TestTypes": "../test-classes/joynr/types/TestTypes",
        "joynr/types/TestTypesWithoutVersion": "../test-classes/joynr/types/TestTypesWithoutVersion",
        "joynr/provisioning": "../test-classes/joynr/provisioning",
        "joynr/vehicle": "../test-classes/joynr/vehicle",
        "test/data": "../test-classes/test/data",
        "integration": "../test-classes/integration"
    },

    shim : {
        'atmosphere' : {
            exports : 'atmosphere'
        }
    },

    deps: allTestFiles,
    callback: window.__karma__.start
});
