/*eslint no-console: "off"*/
/*global req: true*/
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

const Jasmine = require("jasmine");
const jasmine = new Jasmine();
const path = require("path");
const jasminePath = path.join(__dirname, "..", "resources/spec/support/jasmine.json");
jasmine.loadConfigFile(jasminePath);
console.log(`Jasmine version: ${jasmine.jasmine.version}`);

const mod = require("module");
// expose req as a global variable for WebSocketNode (that the mock won't be required)
req = mod.prototype.require;

// load mocks
mod.prototype.require = function(md) {
    if (md.indexOf("Test") === -1) {
        md = md.replace("/global/WebSocketNode", "/../../test/js/global/WebSocketMock");
        md = md.replace("/global/SmrfNode", "/../../test/js/global/SmrfMock");
    }
    return req.call(this, md);
};

if (!module.parent.filename.includes("node-run-unit-tests")) {
    setTimeout(() => {
        console.log("all tests modules loaded");
        jasmine.execute();
    }, 0);
}

module.exports = jasmine;
