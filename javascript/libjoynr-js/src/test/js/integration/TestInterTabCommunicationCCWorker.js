/*global Promise: true, WorkerUtils: true, importScripts: true, joynr: true */

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

// anything that you load here is served through the jsTestDriverServer, if you add an entry you
// have to make it available through the jsTestDriverIntegrationTests.conf
importScripts("WorkerUtils.js");
importScripts("../joynr/provisioning/provisioning_root.js");
importScripts("LocalStorageSimulator.js");

importScripts("../../jar-classes/joynr.intertab.clustercontroller.js");
importScripts("../joynr/provisioning/provisioning_cc.js");

var document = {
    URL : window.joynr.provisioning.brokerUri
};

importScripts("../../classes/lib/bluebird.js");

var Promise = Promise.Promise;

var runtime;

function initializeTest(provisioningSuffix, domain, ownWindow, parentWindow) {
    joynr.provisioning.window = ownWindow;
    joynr.provisioning.parentWindow = parentWindow;
    joynr.provisioning.logging = {
        configuration : {
            name : "TestInterTabCommunicationCCWorkerLogging",
            appenders : {
                WebWorker : {
                    name : "WEBWORKER"
                }
            },
            loggers : {
                root : {
                    level : "debug",
                    AppenderRef : {
                        ref : "WEBWORKER"
                    }
                }
            }
        }
    };

    return joynr.load(joynr.provisioning).then(function(newJoynr) {
        joynr = newJoynr;
        return joynr;
    });
}

function terminateTest() {
    return Promise.resolve();
}

function startTest() {
    return Promise.resolve();
}