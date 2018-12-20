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

console.log("joynr Jasmine 2.x integration tests");
var Jasmine = require("jasmine");
var jasmine = new Jasmine();

jasmine.loadConfigFile(__dirname + "/spec/support/jasmine.json");
var InProcessRuntimeTest = require("../js/browser/start/InProcessRuntimeTest.js");

console.log("all tests modules loaded");
jasmine.execute();
