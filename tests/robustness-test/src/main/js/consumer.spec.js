/*jslint node: true */
/*jslint es5: true, nomen: true */

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

var log = require("./logging.js").log;
var prettyLog = require("./logging.js").prettyLog;

var Promise = require("bluebird").Promise;
var spawn = require('child_process').spawn;

// pares args
if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(1);
}
if (process.env.cmdPath === undefined) {
    log("please pass a cmdPath as argument");
    process.exit(1);
}
var domain = process.env.domain;
var cmdPath = process.env.cmdPath;
var testcase = process.env.testcase;
log("domain: " + domain);
log("testcase: " + testcase);
log("cmdPath: " + cmdPath);

// test results

var tests = [];

// report test result, if called multiple times,
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

var killProvider = function() {
    return new Promise(function(resolve, reject) {
        console.log("Entering killProvider");
        console.log("killProvider: calling " + cmdPath + "/kill-provider.sh");
        var out = spawn(cmdPath + "/kill-provider.sh", []);
        out.stdout.on('data', (data) => {
            console.log(`${data}`);
        });
        out.stderr.on('data', (data) => {
            console.log(`${data}`);
        });
        out.on('close', (code) => {
            console.log("killProvider: in close");
            console.log("killProvider: code = " + code);
            if (code === 0) {
                resolve();
            } else {
                reject(new Error('killProvider failed with code ' + code));
            }
        });
    });
};

var startProviderJs = function() {
    return new Promise(function(resolve, reject) {
        console.log("Entering startProvider");
        var out = spawn(cmdPath + "/start-provider.sh", ['js', domain]);
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
                reject(new Error('startProvider failed with code ' + code));
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

var runTestsWithCppProvider = function(testInterfaceProxy, joynr, onDone) {
    // test implementations

    describe("Robustness test with C++ provider", function() {
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

var runTestsWithJsProvider = function(testInterfaceProxy, joynr, onDone) {
    // test implementations

    describe("Robustness test with JS provider", function() {
        it("proxy is defined", function() {
            expect(testInterfaceProxy).toBeDefined();
        });

        it("callMethodWithStringParameters", function() {
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
            }, "callMethodWithStringParameters", 5000);

            runs(function() {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onError).not.toHaveBeenCalled();
            });
        });

        it("callMethodWithStringParametersAfterProviderHasBeenRestarted", function() {
            var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
            spy.onFulfilled.reset();
            spy.onError.reset();

            runs(function() {
                killProvider().then(function() {
                    return startProviderJs();
                }).then(function() {
                    var args = {
                        stringArg: "Hello"
                    };
                    return testInterfaceProxy.methodWithStringParameters(args);
                }).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "callMethodWithStringParametersAfterProviderHasBeenRestarted", 10000);

            runs(function() {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onError).not.toHaveBeenCalled();
            });
        });

        it("callMethodWithStringParametersBeforeProviderHasBeenRestarted", function() {
            // kill the provider before the request is sent
            var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError" ]);
            spy.onFulfilled.reset();
            spy.onError.reset();

            runs(function() {
                killProvider().then(function() {
                    var args = {
                        stringArg: "Hello"
                    };
                    var result = testInterfaceProxy.methodWithStringParameters(args);
                    startProviderJs().catch(spy.OnError);
                    return result;
                }).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "callMethodWithStringParametersAfterProviderHasBeenRestarted", 10000);

            runs(function() {
                expect(spy.onFulfilled).toHaveBeenCalled();
                expect(spy.onError).not.toHaveBeenCalled();
            });
        });

        it("subscribeToAttributeString", function() {
            var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError" ]);
            var subscriptionId;

            var qosSettings = {
                minIntervalMs: 5000,
                maxIntervalMs: 10000,
                validityMs: 120000
            }
            var subscriptionQosOnChangeWithKeepAlive = new joynr.proxy.OnChangeWithKeepAliveSubscriptionQos(qosSettings);
            spy.onFulfilled.reset();
            spy.onError.reset();
            spy.onPublication.reset();
            spy.onPublicationError.reset();

            runs(function() {
                log("subscribeToAttributeString");
                testInterfaceProxy.attributeString.subscribe({
                    "subscriptionQos": subscriptionQosOnChangeWithKeepAlive,
                    "onReceive": spy.onPublication,
                    "onError": spy.onPublicationError
                }).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "subscribeToAttributeString", 5000);

            runs(function() {
                if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                    log(spy.onError.calls[0].args[0]);
                }
                expect(spy.onFulfilled.callCount).toEqual(1);
                expect(spy.onError.callCount).toEqual(0);
                subscriptionId = spy.onFulfilled.calls[0].args[0];
                log("subscriptionId = " + subscriptionId);
            });

            // the first publication should arrive immediately after subscription is done
            waitsFor(function() {
                // Wait for a subscription message to arrive
                return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
            }, "subscribeToAttributeString Publication", 60000);

            runs(function() {
                expect(spy.onPublication.callCount).toEqual(1);
                expect(spy.onPublicationError.callCount).toEqual(0);
                var retObj = spy.onPublication.calls[0].args[0];
                expect(retObj).toBeDefined();
            });

            // kill and restart the provider while the time period until the next
            // publication happens is passing; the time period must be long enough
            // so that no further publication is sent until the provider got killed
            runs(function() {
                spy.onFulfilled.reset();
                spy.onError.reset();
                killProvider().then(function() {
                    spy.onPublication.reset();
                    spy.onPublicationError.reset();
                    return startProviderJs();
                }).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "subscribeToAttributeString restart provider", 5000);

            runs(function() {
                if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                    log(spy.onError.calls[0].args[0]);
                }
                expect(spy.onFulfilled.callCount).toEqual(1);
                expect(spy.onError.callCount).toEqual(0);
            });

            waitsFor(function() {
                // Wait for a subscription message to arrive
                return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
            }, "subscribeToAttributeString Publication after provider restart", 60000);

            runs(function() {
                expect(spy.onPublication.callCount).toEqual(1);
                expect(spy.onPublicationError.callCount).toEqual(0);
                var retObj = spy.onPublication.calls[0].args[0];
                expect(retObj).toBeDefined();
            });

            runs(function() {
                // unsubscribe again
                spy.onFulfilled.reset();
                spy.onError.reset();
                testInterfaceProxy.attributeString.unsubscribe({
                    "subscriptionId": subscriptionId
                }).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "subscribeToAttributeString unsubscribe", 5000);

            runs(function() {
                if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                    log(spy.onError.calls[0].args[0]);
                }
                expect(spy.onFulfilled.callCount).toEqual(1);
                expect(spy.onError.callCount).toEqual(0);
            });
        });

        it("subscribeToBroadcastWithSingleStringParameter", function() {
            var spy = jasmine.createSpyObj("spy", [ "onFulfilled", "onError", "onPublication", "onPublicationError" ]);
            var subscriptionId;
            var subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({ minInterval: 50 });
            var sleepDone;
            spy.onFulfilled.reset();
            spy.onError.reset();
            spy.onPublication.reset();
            spy.onPublicationError.reset();

            runs(function() {
                log("subscribeToBroadcastWithSingleStringParameter");
                testInterfaceProxy.broadcastWithSingleStringParameter.subscribe({
                    "subscriptionQos": subscriptionQosOnChange,
                    "onReceive": spy.onPublication,
                    "onError": spy.onPublicationError
                }).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "subscribeToBroadcastWithSingleStringParameter", 5000);

            runs(function() {
                if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                    log(spy.onError.calls[0].args[0]);
                }
                expect(spy.onFulfilled.callCount).toEqual(1);
                expect(spy.onError.callCount).toEqual(0);
                subscriptionId = spy.onFulfilled.calls[0].args[0];
                log("subscriptionId = " + subscriptionId);
                // This wait is necessary, because subcriptions are async, and a broadcast could occur
                // before the subscription has started.
                setTimeout(function() {
                    sleepDone = true;
                }, 3000);
            });

            waitsFor(function() {
                return sleepDone;
            }, "subscribeToBroadcastWithSingleStringParameter sleep done", 5000);

            runs(function() {
                spy.onFulfilled.reset();
                spy.onError.reset();
                killProvider().then(function() {
                    return startProviderJs();
                }).then(function() {
                    return testInterfaceProxy.startFireBroadcastWithSingleStringParameter();
                }).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "startFireBroadcastWithSingleStringParameter", 5000);

            runs(function() {
                if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                    log(spy.onError.calls[0].args[0]);
                }
                expect(spy.onFulfilled.callCount).toEqual(1);
                expect(spy.onError.callCount).toEqual(0);
            });

            waitsFor(function() {
                // Wait for a subscription message to arrive
                return spy.onPublication.callCount > 0 || spy.onPublicationError.callCount > 0;
            }, "subscribeToBroadcastWithSingleStringParameter Publication", 60000);

            runs(function() {
                expect(spy.onPublication.callCount).toBeGreaterThan(0);
                expect(spy.onPublicationError.callCount).toEqual(0);

                spy.onFulfilled.reset();
                spy.onError.reset();
                testInterfaceProxy.stopFireBroadcastWithSingleStringParameter(
                        ).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "stopFireBroadcastWithSingleStringParameter", 5000);

            runs(function() {
                if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                    log(spy.onError.calls[0].args[0]);
                }
                expect(spy.onFulfilled.callCount).toEqual(1);
                expect(spy.onError.callCount).toEqual(0);
            });

            runs(function() {
                // unsubscribe again
                spy.onFulfilled.reset();
                spy.onError.reset();
                testInterfaceProxy.broadcastWithSingleStringParameter.unsubscribe({
                    "subscriptionId": subscriptionId
                }).then(spy.onFulfilled).catch(spy.onError);
            });

            waitsFor(function() {
                return spy.onFulfilled.callCount > 0 || spy.onError.callCount > 0;
            }, "subscribeToBroadcastWithSingleStringParameter unsubscribe", 5000);

            runs(function() {
                if (spy.onError.callCount > 0 && spy.onError.calls[0] && spy.onError.calls[0].args[0]) {
                    log(spy.onError.calls[0].args[0]);
                }
                expect(spy.onFulfilled.callCount).toEqual(1);
                expect(spy.onError.callCount).toEqual(0);
            });

        });
    });
};


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
        if (testcase === "js_cpp_tests") {
            console.log("running tests with C++ provider");
            runTestsWithCppProvider(testInterfaceProxy, joynr);
        } else {
            console.log("running tests with JS provider");
            runTestsWithJsProvider(testInterfaceProxy, joynr);
        }
    });
});
