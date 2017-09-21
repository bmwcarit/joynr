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

// run Jasmine 2.x unit tests via node

console.log("joynr Jasmine 2.x node unit tests");

var Jasmine = require("jasmine");
var jasmine = new Jasmine();
jasmine.loadConfigFile("spec/support/jasmine.json");

console.log("Jasmine version: " + jasmine.version);

// because the generated code uses require('joynr') without knowing the location, will work only
// when joynr is a submodule and is placed inside node_modules folder. In order to emulate this
// behavior the require function is adapted here in order to always return the correct joynr while
// running tests.
var mod = require('module');
var joynr = require('../classes/joynr')
var req = mod.prototype.require;
mod.prototype.require = function (md) {
    if (md === 'joynr') {
        return joynr;
    }
    return req.apply(this, arguments);
}
console.log('require config setup');
var InProcessRuntimeTest = require('../test-classes/joynr/start/InProcessRuntimeTest.js');
(function () {
    console.log("all tests modules loaded");
    loadingFinished = true;
    jasmine.execute();
}(InProcessRuntimeTest));
