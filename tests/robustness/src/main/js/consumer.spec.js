/*jslint node: true */
/*jslint es5: true, nomen: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

var Promise = require("joynr/lib/bluebird.js").Promise;
var spawn = require('child_process').spawn;

// test results

var tests = [];

// report test result, if called multple times,
// updates result
var reportTest = function(name, status) {
    console.log("reportTest called: " + name + ", status: " + status);
    var i;
    for (i = 0; i < tests.length; i++) {
        if (tests[i].name === name) {
            tests[i].result = status;
            return;
        }
    }

    // new entry
    var result = { "name" : name, "result" : status };
    tests.push(result);
};

var evaluateAndPrintResults = function() {
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
    for (var i = 0; i < tests.length; i++) {
        result = tests[i].result;
        length = cols - tests[i].name.length - result.length;
        length = (length > 0) ? length : 1;
        output = tests[i].name + filler.substring(0, length) + result;
        log(output);
        if (tests[i].result === "OK") {
            cntOk++;
        } else if (tests[i].result === "FAILED") {
            cntFailed++;
        } else if (tests[i].result === "SKIPPED") {
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

var killClusterController = function() {
    return new Promise(function(resolve, reject) {
        console.log("Entering killClusterController");
        console.log("killClusterController: calling " + cmdPath + "/kill-clustercontroller.sh");
        var out = spawn(cmdPath + "/kill-clustercontroller.sh", []);
        out.stdout.on('data', (data) => {
            console.log(`${data}`);
        });
        out.stderr.on('data', (data) => {
            console.log(`${data}`);
        });
        out.on('close', (code) => {
            console.log("killClusterController: in close");
            console.log("killClusterController: code = " + code);
            if (code === 0) {
                resolve();
            } else {
                reject(new Error('killClusterController failed with code ' + code));
            }
        });
    });
};

var startClusterController = function() {
    return new Promise(function(resolve, reject) {
        console.log("Entering startClusterController");
        var out = spawn(cmdPath + "/start-clustercontroller.sh");
        out.stdout.on('data', (data) => {
            console.log(`${data}`);
        });
        out.stderr.on('data', (data) => {
            console.log(`${data}`);
        });
        out.on('close', (code) => {
            if (code === 0) {
                resolve();
            } else {
                reject(new Error('startClusterController failed with code ' + code));
            }
        });
    });
};

(function() {
    var RobustnessReporter = function() {
    };

    RobustnessReporter.prototype = {
    reportRunnerResults: function(runner) {
            var exitCode = evaluateAndPrintResults();
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
                    reportTest(results.description, "SKIPPED");
                } else {
                    if (results.failedCount > 0) {
                        reportTest(results.description, "FAILED");
                    } else {
                        reportTest(results.description, "OK");
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

    jasmine.RobustnessReporter = RobustnessReporter;
})();

jasmine.getEnv().addReporter(new jasmine.RobustnessReporter());

var runTests = function(testInterfaceProxy, joynr, onDone) {
    // test implementations

    describe("Robustness test", function() {
        it("proxy is defined", function() {
            expect(testInterfaceProxy).toBeDefined();
        });

        it("callMethodBeforeClusterControllerHasBeenRestarted", function() {
            var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
            spy.onFulfilled.reset();
            spy.onError.reset();

            runs(function() {
                var args = {
                    stringArg: "Hello"
                };
                testInterfaceProxy.methodWithStringParameters(args).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "callMethodBeforeClusterControllerHasBeenRestarted", 5000);

            runs(function() {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onError).not.toHaveBeenCalled();
            });
        });

        it("callMethodAfterClusterControllerHasBeenRestarted", function() {
            var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
            spy.onFulfilled.reset();
            spy.onError.reset();

            runs(function() {
                killClusterController().then(function() {
                    return startClusterController();
                }).then(function() {
                    var args = {
                        stringArg: "Hello"
                    };
                    return testInterfaceProxy.methodWithStringParameters(args);
                }).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "callMethodAfterClusterControllerHasBeenRestarted", 10000);

            runs(function() {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onError).not.toHaveBeenCalled();
            });
        });

        it("callMethodWithDelayedResponse", function() {
            var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
            spy.onFulfilled.reset();
            spy.onError.reset();
            var delay = 30000; // in milliseconds, must be shorter than ttl

            runs(function() {
                // call function, on provider side the response is delayed
                // should be sent after the restart of clustercontroller has been finished
                var args = {
                    delayArg : delay
                };
                testInterfaceProxy.methodWithDelayedResponse(args).then(spy.onFulfilled).catch(spy.onError);
                killClusterController().then(function() {
                    return startClusterController();
                }).then(function() {
                    log("ClusterController was restarted successfully");
                }).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "callMethodWithDelayedResponse", delay + 10000);

            runs(function() {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onError).not.toHaveBeenCalled();
            });
        });
    });
};

if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(0);
}
var domain = process.env.domain;
var cmdPath = process.env.cmdPath;
log("domain: " + domain);
log("cmdPath: " + cmdPath);

describe("Consumer test", function() {

    var joynr = require("joynr");
    var provisioning = require("./provisioning_common.js");
    var initialized = false;
    var testInterfaceProxy;

    beforeEach(function() {
        var ready = false;

        if (initialized === false) {

            runs(function() {
                console.log("Environment not yet setup");
                joynr.load(provisioning).then(function(loadedJoynr) {
                    log("joynr started");
                    joynr = loadedJoynr;
                    var messagingQos = new joynr.messaging.MessagingQos({
                        ttl : 60000
                    });
                    var TestInterfaceProxy = require("../generated-javascript/joynr/tests/robustness/TestInterfaceProxy.js");
                    joynr.proxyBuilder.build(TestInterfaceProxy, {
                        domain : domain,
                        messagingQos : messagingQos
                    }).then(function(newTestInterfaceProxy) {
                        testInterfaceProxy = newTestInterfaceProxy;
                        log("testInterface proxy build");
                        ready = true;
                    }).catch(function(error) {
                        log("error building testInterfaceProxy: " + error);
                    });
                }).catch(function(error) {
                    throw error;
                });
            });

            waitsFor(function() {
                return ready;
            }, "joynr proxy robustness", 5000);

            runs(function() {
                initialized = true;
            });
        } else {
            console.log("Environment already setup");
        }
    });

    it("proxy is defined", function() {
        expect(testInterfaceProxy).toBeDefined();
    });

    it("call runTests", function() {
        runTests(testInterfaceProxy, joynr);
    });
});
