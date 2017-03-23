/*global postMessage: true, onmessage: true, document: false, window: false, initializeTest: true, startTest: true, terminateTest: true */
/*jslint es5: true */

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

var self = this, ownWindow, parentWindow, listener, workerId;
var runtime;

var WorkerUtils = (function() {
    var pub = {};

    pub.postReady = function() {
        postMessage({
            type : "ready"
        });
    };

    pub.postStarted = function(argument) {
        var object = {
            type : "started"
        };
        if (argument !== undefined) {
            object.argument = argument;
        }
        postMessage(object);
    };

    pub.postLog = function(msg) {
        postMessage({
            type : "log",
            level : "info",
            message : msg
        });
    };

    pub.postFinished = function() {
        postMessage({
            type : "finished"
        });
    };

    return pub;
}());

/*
 * The ownWindow is used by the web worker to listen to incoming messages in case of intertab communication
 * For testing purposes, we immitate the own window on web worker side, allowing the joynr runtime to register for imcoming events
 * Next, wenn the main frame communicates joynr messages through the web worker postMessage API, these messages will be forwarded to registered event listeners
 */
ownWindow = {
    addEventListener : function(type, callback) {
        if (type === "message") {
            listener = callback;
        }
    },
    removeEventListener : function(type, callback) {
        if (type === "message" && listener === callback) {
            listener = undefined;
        }
    }
};

/*
 * The parent window is used by the web worker to postMessages in case of intertab communication
 * For testing purposes, we immitate the parent window, and directly forward the messages to be sent
 * to the main frame
 * The main frame itself implements the counterpart of this "tunnel", by forwarding incoming
 * messages from the web worker to its own window object
 */
parentWindow = {
    postMessage : function(message, origin) {
        postMessage({
            type : "message",
            data : message.message
        });
    }
};

onmessage =
        function(event) {
            var msg = event.data;
            if (msg.type === "initialize") {
                if (!initializeTest) {
                    throw new Error(
                            "cannot initialize test, worker does not define an initializeTest method");
                }
                workerId = msg.workerId;
                initializeTest(msg.provisioningSuffix, msg.domain, ownWindow, parentWindow).then(
                        function(providedRuntime) {
                            runtime = providedRuntime;
                            WorkerUtils.postReady();
                        });
            } else if (msg.type === "start") {
                if (!startTest) {
                    throw new Error("cannot start test, worker does not define a startTest method");
                }
                startTest().then(function(argument) {
                    WorkerUtils.postStarted(argument);
                });
            } else if (msg.type === "terminate") {

                if (!terminateTest) {
                    throw new Error(
                            "cannot terminate test, worker does not define a terminate method");
                }
                var workerFinished = function() {
                    WorkerUtils.postFinished();
                    self.close();
                };

                terminateTest().then(function() {
                    return runtime.shutdown().then(workerFinished);
                });
            } else if (msg.type === "message") {
                if (listener !== undefined) {
                    listener({
                        data : msg.message
                    });
                }
            }
        };

// window is not available => emulate the window variable through exposure of this, which is the
// DedicatedWorkerContext in case of a WebWorker
var window = this;