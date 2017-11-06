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
var testbase = require("test-base");
var fs = require("fs");
var provisioning = testbase.provisioning_common;
var prettyLog = testbase.logging.prettyLog;
var error = testbase.logging.error;
var log = testbase.logging.log;

if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(-1);
}

if (process.env.cchost === undefined) {
    log("please pass cchost as argument");
    process.exit(-1);
}

if (process.env.ccport === undefined) {
    log("please pass ccport as argument");
    process.exit(-1);
}


log("domain: " + process.env.domain);
log("cchost: " + process.env.cchost);
log("ccport: " + process.env.ccport);
log("ccprotocol: " + process.env.ccprotocol);
log("tlsCertPath: " + process.env.tlsCertPath);
log("tlsKeyPath: " + process.env.tlsKeyPath);
log("tlsCaPath: " + process.env.tlsCaPath);
log("ownerId: " + process.env.ownerId);

var domain = process.env.domain;
provisioning.ccAddress.host = process.env.cchost;
provisioning.ccAddress.port = process.env.ccport;
provisioning.ccAddress.protocol = process.env.ccprotocol;

if (process.env.tlsCertPath || process.env.tlsKeyPath || process.env.tlsCertPath || process.env.ownerId) {
    provisioning.keychain = {};

    if (process.env.tlsCertPath) {
        provisioning.keychain.tlsCert = fs.readFileSync(process.env.tlsCertPath, 'utf8');
    }
    if (process.env.tlsKeyPath) {
        provisioning.keychain.tlsKey = fs.readFileSync(process.env.tlsKeyPath, 'utf8');
    }
    if (process.env.tlsCaPath) {
        provisioning.keychain.tlsCa = fs.readFileSync(process.env.tlsCaPath, 'utf8');
    }
    provisioning.keychain.ownerId = process.env.ownerId;
}

var SystemIntegrationTestProxy = require("../generated-sources/joynr/test/SystemIntegrationTestProxy.js");

var timeout = 600000;

var runTest = function(systemIntegrationTestProxy) {
    var addends = {
        addendA : 123,
        addendB : 321
    };
    return systemIntegrationTestProxy.add(addends).then(function(opArgs){
        if (opArgs.result === undefined) {
            throw new Error("systemIntegrationTestProxy.add failed: result undefined");
        }
        if (opArgs.result !== (addends.addendA + addends.addendB)) {
            log (opArgs.result + " != " + addends.addendA + " + " + addends.addendB);
            throw new Error("systemIntegrationTestProxy.add failed: " + opArgs.result + " != " + addends.addendA + " + " + addends.addendB);
        }
    });
};

joynr.load(provisioning).then(function(loadedJoynr) {
    joynr = loadedJoynr;
    var messagingQos = new joynr.messaging.MessagingQos({
        ttl : 60000
    });

    var discoveryQos = new joynr.proxy.DiscoveryQos({
        discoveryTimeoutMs : 120000 // 2 Mins
    });

    return joynr.proxyBuilder.build(SystemIntegrationTestProxy, {
        domain : domain,
        messagingQos : messagingQos,
        discoveryQos : discoveryQos
    }).then(function(systemIntegrationTestProxy) {
        return runTest(systemIntegrationTestProxy).then(function(){
            log("SIT RESULT success: node consumer -> " + domain);
            process.exit(0);
        });
    }).catch(function(error) {
        log("SIT RESULT error: node consumer -> " + domain + " error: " + JSON.stringify(error));
        process.exit(1);
    });
}).catch(function(error){
    throw error;
});
