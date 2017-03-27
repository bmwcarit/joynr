/*jslint node: true */

/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

var Promise = require("bluebird").Promise;
var joynr = require("joynr");
var prettyLog = require("test-base").logging.prettyLog;

exports.implementation = {
    add : function(opArgs) {
        prettyLog("SystemIntegrationTestProvider.add(" + JSON.stringify(opArgs) + ") called");
        return new Promise(function(resolve, reject) {
            if (opArgs.addendA === undefined || opArgs.addendB === undefined) {
                reject(new joynr.exceptions.ProviderRuntimeException(
                        {detailMessage: "add: invalid argument data"}));
            } else {
                resolve({result: opArgs.addendA + opArgs.addendB});
            }
        });
    }
};

self = exports.implementation;
