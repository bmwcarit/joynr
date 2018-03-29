/*jslint es5: true, node: true, nomen: true */
/*global req: true */
/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

// run Jasmine 2.x unit tests via node

function loadJasmine() {
    console.log("joynr Jasmine 2.x node unit tests");

    var Jasmine = require("jasmine");
    var jasmine = new Jasmine();
    jasmine.loadConfigFile(__dirname + "/../resources/spec/support/jasmine.json");

    console.log("Jasmine version: " + jasmine.version);

    // because the generated code uses require('joynr') without knowing the location, it will work only
    // when joynr is a submodule and is placed inside node_modules folder. In order to emulate this
    // behavior the require function is adapted here in order to always return the correct joynr while
    // running tests.
    var mod = require("module");
    var joynr = require("../../main/js/joynr");
    var unmodifiedRequire = mod.prototype.require;
    // expose req as a global variable for WebSocketNode (that the mock won't be required)
    req = function(md) {
        if (md === "joynr") {
            return joynr;
        }

        md = md.replace("/classes/joynr/types", "/../main/generated/joynr/types");
        md = md.replace("/classes/joynr/system/RoutingTypes", "/../main/generated/joynr/system/RoutingTypes");
        md = md.replace("/classes/joynr", "/../main/js/joynr");
        md = md.replace("/classes/lib", "/../../../js-dependencies/src/main/js/uuid");
        md = md.replace("/classes/global", "/../main/js/global");
        md = md.replace("/test-classes/joynr/vehicle", "/generated/joynr/vehicle");
        md = md.replace("/test-classes/joynr/tests", "/generated/joynr/tests");
        md = md.replace("/test-classes/joynr/datatypes", "/generated/joynr/datatypes");
        md = md.replace("/test-classes/joynr/provisioning", "/resources/joynr/provisioning");
        md = md.replace("/test-classes/global", "/../test/js/global");
        md = md.replace("/test-classes/test", "/../test/js/test");
        md = md.replace("/test-classes/joynr/types", "/generated/joynr/types");
        md = md.replace("../joynr/types/", "../../generated/joynr/types/");
        md = md.replace(
            "/generated/joynr/types/ArbitrationStrategyCollection",
            "/js/joynr/types/ArbitrationStrategyCollection"
        );
        md = md.replace("/generated/joynr/types/TypeRegistrySingleton", "/js/joynr/types/TypeRegistrySingleton");
        md = md.replace("../lib/atmosphereNode", "../../resources/lib/atmosphereNode");
        md = md.replace("../lib/JsonParser", "/../../../../../js-dependencies/src/main/js/JsonParser/JsonParser");
        md = md.replace("/../lib", "/../../../../../js-dependencies/src/main/js/uuid");
        md = md.replace("../../infrastructure", "../../../../../main/generated/joynr/infrastructure");
        md = md.replace("../system/RoutingTypes", "../../../../main/generated/joynr/system/RoutingTypes");
        md = md.replace("../system/Discovery", "../../../../main/generated/joynr/system/Discovery");
        md = md.replace("../system/Routing", "../../../../main/generated/joynr/system/Routing");
        md = md.replace("../types/Discovery", "../../../../main/generated/joynr/types/Discovery");

        return unmodifiedRequire.call(this, md);
    };

    // override mocks
    mod.prototype.require = function(md) {
        if (md === "joynr") {
            return joynr;
        }

        if (md.endsWith("SmrfNode")) {
            return req("../js/global/SmrfMock");
        }
        if (md.endsWith("WebSocketNode")) {
            return req("../js/global/WebSocketMock");
        }
        return req.apply(this, arguments);
    };

    console.log("require config setup");

    setTimeout(function() {
        console.log("all tests modules loaded");
        jasmine.execute();
    }, 0);
}
module.exports = __dirname.includes("target") ? undefined : loadJasmine();
