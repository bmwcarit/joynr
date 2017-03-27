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

console.log("Running addReporter.js");

var reporters = require('jasmine-reporters');

// JUnit reporter
var junitReporter = new reporters.JUnitXmlReporter({
    savePath: 'jstd-test-results',
    consolidateAll: true,
    filePrefix: 'TEST-unit-node'
});

jasmine.getEnv().addReporter(junitReporter);


// Terminal reporter
var terminalReporter = new reporters.TerminalReporter({
    verbosity: 3,   // max 3, only 3 provides info about passed tests
    color: true,
    showStack: true
});
jasmine.getEnv().addReporter(terminalReporter);
