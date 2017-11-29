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

var joynr = require("joynr");
var testbase = require("test-base");
var fs = require("fs");
var log = testbase.logging.log;
var provisioning = testbase.provisioning_common;

if (process.env.domain === undefined) {
    log("please pass a domain as argument");
    process.exit(0);
}

if (process.env.cchost === undefined) {
    log("please pass cchost as argument");
    process.exit(0);
}

if (process.env.ccport === undefined) {
    log("please pass ccport as argument");
    process.exit(0);
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

joynr.load(testbase.provisioning_common).then(function(loadedJoynr) {
    log("joynr started");
    joynr = loadedJoynr;

    var providerQos = new joynr.types.ProviderQos({
        customParameters : [],
        priority : Date.now(),
        scope : joynr.types.ProviderScope.GLOBAL,
        supportsOnChangeSubscriptions : true
    });

    var SystemIntegrationTestProvider = require("../generated-sources/joynr/test/SystemIntegrationTestProvider.js");
    var SystemIntegrationTestProviderImpl = require("./SystemIntegrationTestProviderImpl.js");
    var systemIntegrationTestProvider = joynr.providerBuilder.build(
            SystemIntegrationTestProvider,
            SystemIntegrationTestProviderImpl.implementation);

    joynr.registration.registerProvider(domain, systemIntegrationTestProvider, providerQos).then(function() {
        log("provider registered successfully");
    }).catch(function(error) {
        log("error registering provider: " + error.toString());
    });
    return loadedJoynr;
}).catch(function(error) {
    throw error;
});
