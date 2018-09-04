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

const Jasmine = require("jasmine");
const jasmine = new Jasmine();
jasmine.loadConfigFile(`${__dirname}/spec/support/jasmine.json`);

const CompatibleProvidersTest = require("../js/node_integration/consumer/CompatibleProvidersTest.js");
const End2EndRPCTest = require("../js/node_integration/consumer/End2EndRPCTest.js");
const End2EndSubscriptionTest = require("../js/node_integration/consumer/End2EndSubscriptionTest.js");
const End2EndDatatypesTest = require("../js/node_integration/consumer/End2EndDatatypesTest.js");
const IncompatibleProviderTest = require("../js/node_integration/consumer/IncompatibleProviderTest.js");
const LocalDiscoveryTest = require("../js/node_integration/consumer/LocalDiscoveryTest");
const MultipleVersionsTest = require("../js/node_integration/consumer/MultipleVersionsTest");

console.log("all tests modules loaded");
jasmine.execute();
