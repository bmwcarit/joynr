/*jslint es5: true, node: true */
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

var Promise = require("../../classes/global/Promise");
var provisioning_root = require("joynr/provisioning/provisioning_root");
var waitsFor = require("../global/WaitsFor");
var child_process = require("child_process");

var IntegrationUtils = {};
var currentlyRunningChildCC;
var childReady = {},
    childStarted = {},
    childFinished = {},
    child = {},
    processId = 0,
    queuedLogs = {};
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
        ttl: provisioning_root.ttl
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
    console.log(error);
    expect(error.toString()).toBeFalsy();
};

IntegrationUtils.getObjectType = function getObjectType(obj) {
    if (obj === null || obj === undefined) {
        throw new Error("cannot determine the type of an undefined object");
    }
    var funcNameRegex = /function ([$\w]+)\(/;
    var results = funcNameRegex.exec(obj.constructor.toString());
    return results && results.length > 1 ? results[1] : "";
};

IntegrationUtils.checkValueAndType = function checkValueAndType(arg1, arg2) {
    expect(arg1).toEqual(arg2);
    expect(typeof arg1).toEqual(typeof arg2);
    expect(IntegrationUtils.getObjectType(arg1)).toEqual(IntegrationUtils.getObjectType(arg2));
};
IntegrationUtils.createPromise = function createPromise() {
    var map = {};
    map.promise = new Promise(function(resolve, reject) {
        map.resolve = resolve;
        map.reject = reject;
    });

    return map;
};

IntegrationUtils.initializeChildProcess = function(childName, provisioningSuffix, domain, processSpecialization, cc) {
    processId++;
    var newChildId = processId;
    if (cc) {
        currentlyRunningChildCC = newChildId;
    }
    childReady[processId] = IntegrationUtils.createPromise();
    childStarted[processId] = IntegrationUtils.createPromise();
    childFinished[processId] = IntegrationUtils.createPromise();

    var processConfig = process.env.debugPort ? { execArgv: ["--inspect-brk=" + process.env.debugPort] } : {};

    var forked = child_process.fork("./test-classes/node_integration/" + childName + ".js", [], processConfig);
    forked.on("message", function(msg) {
        // Handle messages from child process
        console.log("received message: " + JSON.stringify(msg));
        if (msg.type === "ready") {
            childReady[newChildId].resolve(newChildId);
        } else if (msg.type === "started") {
            childStarted[newChildId].resolve(msg.argument);
        } else if (msg.type === "finished") {
            childFinished[newChildId].resolve(true);
        }
    });
    child[newChildId] = forked;
    child[newChildId].send({
        type: "initialize",
        provisioningSuffix: provisioningSuffix,
        domain: domain,
        processSpecialization: processSpecialization
    });

    return childReady[newChildId].promise;
};

IntegrationUtils.startChildProcess = function(newChildId) {
    child[newChildId].send({
        type: "start"
    });
    return childStarted[newChildId].promise;
};

IntegrationUtils.buildProxy = function buildProxy(ProxyConstructor, domain) {
    if (domain === undefined) {
        throw new Error("specify domain");
    }
    return joynr.proxyBuilder
        .build(ProxyConstructor, {
            domain: domain,
            messagingQos: IntegrationUtils.messagingQos
        })
        .catch(IntegrationUtils.outputPromiseError);
};

IntegrationUtils.shutdownChildProcess = function(childId) {
    // signal child to shut down
    var promise = null;
    if (child[childId]) {
        child[childId].send({
            type: "terminate"
        });

        // wait for child to be shut down
        childFinished[childId].promise.then(function() {
            if (currentlyRunningChildCC === childId) {
                currentlyRunningChildCC = undefined;
            }
            child[childId].kill();
            child[childId] = undefined;
            childReady[childId] = undefined;
            childStarted[childId] = undefined;
            childFinished[childId] = undefined;
        });

        promise = childFinished[childId].promise;
    } else {
        promise = new Promise(function(resolve) {
            resolve();
        });
    }

    return promise;
};

IntegrationUtils.shutdownLibjoynr = function() {
    var promise = joynr.shutdown();

    promise.catch(function(error) {
        IntegrationUtils.outputPromiseError(error);
    });
    return promise;
};

IntegrationUtils.waitALittle = function waitALittle(time) {
    var start;
    start = Date.now();

    // wait for childProcess to be shut down
    return waitsFor(
        function() {
            return Date.now() - start > time;
        },
        time + " ms to elapse",
        time
    );
};

module.exports = IntegrationUtils;
