/*jslint node: true */
/*jslint es5: true, nomen: true */

/*
 * #%L
 * %%
 * Copyright (C) 2015 - 2017 BMW Car IT GmbH
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

var log = require("./logging.js").log;
var prettyLog = require("./logging.js").prettyLog;
var errorLog = require("./logging.js").prettyLog;

var testReporter = {};

// test results
testReporter.tests = [];

// report test result, if called multiple times,
// updates result
testReporter.reportTest = function(name, status) {
    log("reportTest called: " + name + ", status: " + status);
    var i;
    for (i = 0; i < testReporter.tests.length; i++) {
        if (testReporter.tests[i].name === name) {
            testReporter.tests[i].result = status;
            return;
        }
    }

    // new entry
    var result = { "name" : name, "result" : status };
    testReporter.tests.push(result);
};

testReporter.evaluateAndPrintResults = function() {
    var exitCode;
    var cntFailed = 0;
    var cntSkipped = 0;
    var cntOk = 0;
    var cols = 75;
    var buffer = '===========================================================================';
    var filler = '...........................................................................';
    var horizontalRuler = buffer;
    var result;
    var length;
    var output;
    log(horizontalRuler);
    log("INTERLANGUAGE TEST SUMMARY (JAVASCRIPT CONSUMER):");
    log(horizontalRuler);
    for (var i = 0; i < testReporter.tests.length; i++) {
        result = testReporter.tests[i].result;
        length = cols - testReporter.tests[i].name.length - result.length;
        length = (length > 0) ? length : 1;
        output = testReporter.tests[i].name + filler.substring(0, length) + result;
        log(output);
        if (testReporter.tests[i].result === "OK") {
            cntOk++;
        } else if (testReporter.tests[i].result === "FAILED") {
            cntFailed++;
        } else if (testReporter.tests[i].result === "SKIPPED") {
            cntSkipped++;
        }
    }
    log(horizontalRuler);
    log("Tests executed: " + (cntOk + cntFailed) + ", Success: " + cntOk + ", Failures: " + cntFailed + ", Skipped: " + cntSkipped);
    log(horizontalRuler);
    if (cntFailed > 0) {
        log("Final result: FAILED");
        exitCode = 1;
    } else {
        log("Final result: SUCCESS");
        exitCode = 0;
    }
    log(horizontalRuler);
    return exitCode;
};

(function() {
    var TestReporter = function() {
    };

    TestReporter.prototype = {
        reportRunnerResults: function(runner) {
                var exitCode = testReporter.evaluateAndPrintResults();
                /*
                 * for some unknown reason, the jasmine test does not
                 * exit, but keeps on hanging. Thus do the exit manually
                 * here as a workaround.
                 * NOTE: Using the parameter --forceexit does not work
                 * either, since then the "reportRunnerResults" is not
                 * getting called.
                 */
                process.exit(exitCode);
        },
    
        reportRunnerStarting: function(runner) {
                // intentionally left empty
        },
    
        reportSpecResults: function(spec) {
                var results = spec.results();
                if (results) {
                    if (results.skipped) {
                        testReporter.reportTest(results.description, "SKIPPED");
                    } else {
                        if (results.failedCount > 0) {
                            testReporter.reportTest(results.description, "FAILED");
                        } else {
                            testReporter.reportTest(results.description, "OK");
                        }
                    }
                }
        },
    
        reportSpecStarting: function(spec) {
                // intentionally left empty
        },
    
        reportSuiteResults: function(suite) {
                // intentionally left empty
        }
    };

    testReporter.TestReporter = TestReporter;
})();

module.exports = testReporter;