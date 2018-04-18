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
var Promise = require('../../classes/global/Promise');
var provisioning_root = require('../../test-classes/joynr/provisioning/provisioning_root');
var provisioning_end2end = require('./provisioning_end2end_common');
var defaultInterTabSettings = require('../../classes/joynr/start/settings/defaultInterTabSettings');
var waitsFor = require('../../test-classes/global/WaitsFor');

            var IntegrationUtils = {};
            var currentlyRunningWebWorkerCC;
            var workerReady = {}, workerStarted = {}, workerFinished = {}, worker = {}, workerId =
                    0, queuedLogs = {};
            var joynr;

            IntegrationUtils.log = function log(msg, id) {
                if (joynr !== undefined) {
                    var logger = joynr.logging.getLogger(id);
                    logger.log(joynr.logging.getLogLevel(msg.level), [msg.message]);
                } else {
                    if (queuedLogs[id] === undefined) {
                        queuedLogs[id] = [];
                    }
                    queuedLogs[id].push(msg);
                }
            };

            IntegrationUtils.initialize = function initialize(asynclib) {
                joynr = asynclib;
                IntegrationUtils.messagingQos = new joynr.messaging.MessagingQos({
                    ttl : provisioning_root.ttl
                });

                var queuedMsgs, id, msg;
                for (id in queuedLogs) {
                    if (queuedLogs.hasOwnProperty(id)) {
                        queuedMsgs = queuedLogs[id];
                        for (msg in queuedMsgs) {
                            if (queuedMsgs.hasOwnProperty(msg)) {
                                IntegrationUtils.log(queuedMsgs[msg], id);
                            }
                        }
                    }
                }
            };

            IntegrationUtils.outputPromiseError = function outputPromiseError(error) {
                expect(error.toString()).toBeFalsy();
            };

            IntegrationUtils.getObjectType = function getObjectType(obj) {
                if (obj === null || obj === undefined) {
                    throw new Error("cannot determine the type of an undefined object");
                }
                var funcNameRegex = /function ([$\w]+)\(/;
                var results = funcNameRegex.exec(obj.constructor.toString());
                return (results && results.length > 1) ? results[1] : "";
            };

            IntegrationUtils.checkValueAndType =
                    function checkValueAndType(arg1, arg2) {
                        expect(arg1).toEqual(arg2);
                        expect(typeof arg1).toEqual(typeof arg2);
                        expect(IntegrationUtils.getObjectType(arg1)).toEqual(
                                IntegrationUtils.getObjectType(arg2));
                    };

            IntegrationUtils.newWebWorker = function newWebWorker(workerName, onmessage) {
                var worker = new Worker("/base/test-classes/integration/" + workerName + ".js");
                worker.onmessage = onmessage;
                worker.onerror = function(error) {
                    // workaround to show web worker errors: thrown errors do not interfere
                    // with the test execution
                    expect(workerName + " error: " + error.message).toBeFalsy();
                };
                return worker;
            };

            IntegrationUtils.createPromise =
                    function createPromise() {
                        var map = {};
                        map.promise = new Promise(function(resolve, reject) {
                            map.resolve = resolve;
                            map.reject = reject;
                        });
                        /*
                         * this seems not to work in the es6-promise implementation, v 2.0.1
                         * map.then = map.promise.then;
                         */
                        return map;
                    };

            IntegrationUtils.initializeWebWorker =
                    function initializeWebWorker(workerName, provisioningSuffix, domain, cc) {
                        // initialize web worker
                        workerId++;
                        var newWorkerId = workerId;
                        if (cc) {
                            currentlyRunningWebWorkerCC = newWorkerId;
                        }
                        workerReady[workerId] = IntegrationUtils.createPromise();
                        workerStarted[workerId] =
                                IntegrationUtils.createPromise();
                        workerFinished[workerId] =
                                IntegrationUtils.createPromise();
                        worker[workerId] =
                                IntegrationUtils.newWebWorker(workerName, function(event) {
                                    var msg = event.data;
                                    if (msg.type === "ready") {
                                        workerReady[newWorkerId].resolve(newWorkerId);
                                    } else if (msg.type === "log") {
                                        var id =
                                                workerName
                                                    + "."
                                                    + provisioningSuffix
                                                    + "."
                                                    + domain;
                                        IntegrationUtils.log(msg, "joynr.webworker." + workerName, provisioningSuffix, domain);
                                    } else if (msg.type === "started") {
                                        workerStarted[newWorkerId].resolve(msg.argument);
                                    } else if (msg.type === "finished") {
                                        workerFinished[newWorkerId].resolve(true);
                                    } else if (msg.type === "message") {
                                        /*
                                         * In case the webworker transmits joynr messages to the
                                         * mainframe, we forward this messages directly to the local
                                         * window object In this case, event listeners for the local
                                         * window object get notified about the incoming message
                                         */
                                        window
                                                .postMessage(
                                                        msg.data,
                                                        defaultInterTabSettings.parentOrigin);
                                    }
                                });
                        worker[newWorkerId].postMessage({
                            type : "initialize",
                            workerId : newWorkerId,
                            provisioningSuffix : provisioningSuffix,
                            domain : domain
                        });

                        return workerReady[newWorkerId].promise;
                    };

            IntegrationUtils.initializeWebWorkerCC =
                    function(workerName, provisioningSuffix, domain) {
                        if (currentlyRunningWebWorkerCC !== undefined) {
                            throw new Error(
                                    "trying to start new web worker for cluster controller despite old cluster controller has not been terminated");
                        } else {
                            return IntegrationUtils.initializeWebWorker(
                                    workerName,
                                    provisioningSuffix,
                                    domain,
                                    true);
                        }
                    };
            IntegrationUtils.startWebWorker = function startWebWorker(newWorkerId) {
                worker[newWorkerId].postMessage({
                    type : "start"
                });
                return workerStarted[newWorkerId].promise;
            };

            IntegrationUtils.getWorkerReadyStatus = function getWorkerReadyStatus(newWorkerId) {
                return workerReady[newWorkerId].promise;
            };

            IntegrationUtils.getWorker = function getWorker(newWorkerId) {
                if (newWorkerId) {
                    return worker[newWorkerId];
                }
            };

            IntegrationUtils.getCCWorker = function getCCWorker() {
                return worker[currentlyRunningWebWorkerCC];
            };

            IntegrationUtils.getProvisioning =
                    function getProvisioning(provisioning, identifier) {
                        var window = provisioning.window;
                        var parentWindow = provisioning.parentWindow;

                        // set to null when copying.
                        provisioning.window = null;
                        provisioning.parentWindow = null;

                        // this form of deep copy is very fast, but object size limited.
                        var provisioningCopy = JSON.parse(JSON.stringify(provisioning));
                        provisioningCopy.channelId = identifier;
                        provisioningCopy.window = window;
                        provisioningCopy.parentWindow = parentWindow;

                        // put window objects back on
                        provisioning.window = window;
                        provisioning.parentWindow = parentWindow;

                        return provisioningCopy;
                    };

            IntegrationUtils.buildProxy = function buildProxy(ProxyConstructor, domain) {
                if (domain === undefined) {
                    throw new Error("specify domain");
                }
                return joynr.proxyBuilder.build(ProxyConstructor, {
                    domain : domain,
                    messagingQos : IntegrationUtils.messagingQos
                }).catch(IntegrationUtils.outputPromiseError);
            };

            IntegrationUtils.shutdownWebWorker = function shutdownWebWorker(newWorkerId) {
                // signal worker to shut down
                var promise = null;
                if (worker[newWorkerId]) {
                    worker[newWorkerId].postMessage({
                        type : "terminate"
                    });

                    // wait for worker to be shut down
                    workerFinished[newWorkerId].promise.then(function() {
                        if (currentlyRunningWebWorkerCC === newWorkerId) {
                            currentlyRunningWebWorkerCC = undefined;
                        }
                        worker[newWorkerId].terminate();
                        worker[newWorkerId] = undefined;
                        workerReady[newWorkerId] = undefined;
                        workerStarted[newWorkerId] = undefined;
                        workerFinished[newWorkerId] = undefined;
                    });

                    promise = workerFinished[newWorkerId].promise;
                } else {
                    promise = new Promise(function(resolve) {
                        resolve();
                    });
                }

                return promise;
            };

            IntegrationUtils.shutdownLibjoynr = function shutdownLibjoynr() {
                var promise = joynr.shutdown();

                promise.catch(function(error) {
                    IntegrationUtils.outputPromiseError(error);
                });
                return promise;
            };

            IntegrationUtils.waitALittle = function waitALittle(time) {
                var start;
                start = Date.now();

                // wait for worker to be shut down
                return waitsFor(function() {
                    return Date.now() - start > time;
                }, time + " ms to elapse", time);
            };

            module.exports = IntegrationUtils;
