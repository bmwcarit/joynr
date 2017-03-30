/*jslint node: true */

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

var prettyLog = require("./logging.js").prettyLog;
var error = require("./logging.js").error;

var Promise = require("bluebird").Promise;

var joynr = require("joynr");

var robustnessProvider;

// Attributes
var attributeStringValue = "done";
var intervalTimer;

exports.setProvider = function(provider) {
    robustnessProvider = provider;
};

function sendBroadcast(self) {
    var stringOut = "boom";
    var outputParameters = self.broadcastWithSingleStringParameter.createBroadcastOutputParameters();
    outputParameters.setStringOut(stringOut);
    self.broadcastWithSingleStringParameter.fire(outputParameters);
}

exports.implementation = {
    // attribute getter and setter
    attributeString : {
        get : function() {
            prettyLog("RobustnessProvider.attributeString.get() called");
            return new Promise(function(resolve, reject) {
                resolve(attributeStringValue);
            });
        },
        set : function(value) {
            prettyLog("RobustnessProvider.attributeString.set(" + value + ") called");
            attributeStringValue = value;
            self.attributeString.valueChanged(attributeStringValue);
            return new Promise(function(resolve, reject) {
                resolve();
            });
        }
    },

    methodWithStringParameters : function(opArgs) {
        prettyLog("RobustnessProvider.methodWithStringParameters(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.stringArg === undefined || opArgs.stringArg === null) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithStringParameters: invalid argument stringArg"}));
            } else {
                resolve({
                    stringOut: "received stringArg: " + opArgs.stringArg
                });
            }
        });
    },

    methodWithDelayedResponse : function(opArgs) {
        prettyLog("RobustnessProvider.methodWithDelayedResponse(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.delayArg === undefined) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "methodWithDelayedResponse: invalid argument delayArg"}));
            } else {
                // do the delay here
                setTimeout(function delayTimer() {
                    resolve({
                        stringOut: "done"
                    });
                }, delayArg);
            }
        });
    },

	// BROADCASTS aka events

    broadcastWithSingleStringParameter : {},

    methodToFireBroadcastWithSingleStringParameter : function() {
        prettyLog("RobustnessProvider.methodToFireBroadcastWithSingleStringParameter() called");
        return new Promise(function(resolve, reject) {
            sendBroadcast(self);
            resolve();
        });
    },

    startFireBroadcastWithSingleStringParameter : function() {
        prettyLog("RobustnessProvider.startFireBroadcastWithSingleStringParameter() called");
        return new Promise(function(resolve, reject) {
            if (intervalTimer) {
                clearTimeout(intervalTimer);
                intervalTimer = undefined;
            }
            var numberOfBroadcasts = 0;
            var periodMs = 250;
            var validityMs = 60000;
            intervalTimer = setInterval(function() {
                sendBroadcast(self);
                numberOfBroadcasts++;
                if (numberOfBroadcasts === (validityMs / periodMs)) {
                    clearInterval(intervalTimer);
                }
            }, periodMs);
            if (intervalTimer) {
                // intervalTimer successfully started
                resolve();
            } else {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "startFireBroadcastWithSingleStringParameter: intervalTimer could not be started"}));
            }
        });
    },

    stopFireBroadcastWithSingleStringParameter : function() {
        prettyLog("RobustnessProvider.stopFireBroadcastWithSingleStringParameter() called");
        return new Promise(function(resolve, reject) {
            if (intervalTimer) {
                clearInterval(intervalTimer);
                intervalTimer = undefined;
                resolve();
            } else {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "stopFireBroadcastWithSingleStringParameter: no intervalTimer running"}));
            }
        });
    }
};

self = exports.implementation;
