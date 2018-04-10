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

console.log("joynr Jasmine 2.x system integration tests");

var Jasmine = require("jasmine");
var jasmine = new Jasmine();
jasmine.loadConfigFile("spec/support/jasmine.json");

console.log("Jasmine version: " + jasmine.version);

// because the generated code uses require('joynr') without knowing the location, will work only
// when joynr is a submodule and is placed inside node_modules folder. In order to emulate this
// behavior the require function is adapted here in order to always return the correct joynr while
// running tests.
var mod = require("module");
var joynr = require("../classes/joynr");
var path = require("path");
var req = mod.prototype.require;
mod.prototype.require = function(md) {
    if (md === "joynr") {
        return joynr;
    }

    // mock localStorage
    if (md.endsWith("LocalStorageNode")) {
        var appDir = path.dirname(require.main.filename);
        return req.call(this, appDir + "/node_integration/LocalStorageMock.js");
    }

    if (
        md.startsWith("joynr/vehicle") ||
        md.startsWith("joynr/datatypes") ||
        md.startsWith("joynr/tests") ||
        md.startsWith("joynr/provisioning")
    ) {
        return req.call(this, "../" + md);
    }

    if (md.startsWith("joynr")) {
        return req.call(this, "../../classes/" + md);
    }

    if (md === "joynr/Runtime") {
        return req.call(this, "joynr/Runtime.inprocess");
    }
    return req.apply(this, arguments);
};
console.log("require config setup");
var CompatibleProvidersTest = require("../test-classes/node_integration/CompatibleProvidersTest.js");
var End2EndRPCTest = require("../test-classes/node_integration/End2EndRPCTest.js");
var End2EndSubscriptionTest = require("../test-classes/node_integration/End2EndSubscriptionTest.js");
var End2EndDatatypesTest = require("../test-classes/node_integration/End2EndDatatypesTest.js");
var IncompatibleProviderTest = require("../test-classes/node_integration/IncompatibleProviderTest.js");
var LocalDiscoveryTest = require("../test-classes/node_integration/LocalDiscoveryTest");
console.log("all tests modules loaded");
jasmine.execute();
