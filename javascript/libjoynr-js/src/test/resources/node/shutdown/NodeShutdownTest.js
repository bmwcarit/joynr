/*jslint es5: true, node: true */
/*global console:true */
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
var exitHook = require('exit-hook');
var joynr = require('joynr');
var Promise = require('bluebird').Promise;
var provisioning = { ccAddress: { host : "localhost", port : 4242 } };

var timeStart;
var SHUTDOWN_MAX_TIME = 5000;

exitHook(function () {
    var timeSpent = Date.now() - timeStart;
    if (timeSpent > SHUTDOWN_MAX_TIME) {
        console.log("Shutdown of libjoynr took longer then expected: " + timeSpent + "ms instead of max " + SHUTDOWN_MAX_TIME + "ms");
        process.exit(-1);
    }
    console.log("Shutdown of libjoynr succeeded within " + timeSpent + "ms");
});

joynr.load(provisioning).then(function(loadedJoynr) {
    return new Promise(function(resolve, reject) {
        setTimeout(function() {
            loadedJoynr.shutdown().then(function() {
                timeStart = Date.now();
                resolve();
            }).catch(reject);
        }, 100);
    });
}).catch(function(error) {
    console.log("Error from joynr.load: " + error);
    process.exit(-1);
});

