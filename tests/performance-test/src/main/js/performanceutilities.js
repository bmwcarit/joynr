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

PerformanceUtilities = {};

PerformanceUtilities.createByteArray = function(size, defaultValue) {
    result = [];

    for(var i=0 ; i < size ; i++) {
        result.push(defaultValue);
    }

    return result;
};

PerformanceUtilities.createString = function(length, defaultChar) {
    // Add one because the character is inserted between the array elements.
    return String(new Array(length + 1).join(defaultChar))
};

PerformanceUtilities.createRandomNumber = function createRandomNumber(max) {
    return Math.floor(Math.random()*(max+1));
};

/**
 * Reads command line arguments from environment. If an argument is not
 * available, a default value will be used.
 */
PerformanceUtilities.getCommandLineOptionsOrDefaults = function(environment) {
    var bounceProxyBaseUrl, domain, stringLength, byteArrayLength, numRuns, timeout, brokerUri, viacc, cchost, ccport, host, skipByteArraySizeTimesK;

    if(environment.domain != undefined) {
        domain = environment.domain;
    } else {
        domain = "test_domain";
    }

    if(environment.stringlength != undefined) {
        stringLength = environment.stringlength;
    } else {
        stringLength = 10;
    }

    if(environment.bytearraylength != undefined) {
        byteArrayLength = environment.bytearraylength;
    } else {
        byteArrayLength = 100;
    }

    if(environment.runs != undefined) {
        numRuns = environment.runs;
    } else {
        numRuns = 10000;
    }

    if(environment.timeout != undefined) {
        timeout = environment.timeout;
    } else {
        timeout = 3600000;
    }

    if(environment.viacc != undefined) {
        viacc = environment.viacc;
    } else {
        viacc = 'true';
    }

    if(environment.brokerUri != undefined) {
        brokerUri = environment.brokerUri;
    } else {
        brokerUri = 'tcp://localhost:1883';
    }

    if(environment.bounceProxyBaseUrl != undefined) {
        bounceProxyBaseUrl = environment.bounceProxyBaseUrl;
    } else {
        bounceProxyBaseUrl = 'http://localhost:8080';
    }

    if(environment.cchost != undefined) {
        cchost = environment.cchost;
    } else {
        cchost = 'localhost';
    }

    if(environment.ccport != undefined) {
        ccport = environment.ccport;
    } else {
        ccport = 4242;
    }

    if(environment.skipByteArraySizeTimesK != undefined) {
        skipByteArraySizeTimesK = environment.skipByteArraySizeTimesK;
    } else {
        skipByteArraySizeTimesK = false;
    }

    return {
        'stringLength' : stringLength,
        'byteArrayLength' : byteArrayLength,
        'numRuns' : numRuns,
        'timeout' : timeout,
        'domain' : domain,
        'brokerUri' : brokerUri,
        'viacc' : viacc,
        'cchost': cchost,
        'ccport': ccport,
        'bounceProxyBaseUrl': bounceProxyBaseUrl,
        'skipByteArraySizeTimesK': skipByteArraySizeTimesK
    };
};

module.exports = PerformanceUtilities;
