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

var PerformanceUtilities = {};

PerformanceUtilities.createByteArray = function(size, defaultValue) {
    var result = [];

    for (var i = 0 ; i < size ; i++) {
        result.push(defaultValue);
    }

    return result;
};

PerformanceUtilities.createString = function(length, defaultChar) {
    // Add one because the character is inserted between the array elements.
    return String(new Array(length + 1).join(defaultChar));
};

PerformanceUtilities.createRandomNumber = function createRandomNumber(max) {
    return Math.floor(Math.random() * (max + 1));
};

PerformanceUtilities.forceGC = function(){
    if (global.gc){
        global.gc();
    } else {
        console.error('no gc hook! (Start node with --expose-gc  -> use npm run startconsumer)');
    }
};

/**
 * Reads command line arguments from environment. If an argument is not
 * available, a default value will be used.
 */
PerformanceUtilities.getCommandLineOptionsOrDefaults = function(environment) {
    var bounceProxyBaseUrl, domain, stringLength, byteArrayLength, numRuns, timeout, brokerUri, viacc, cchost, ccport,
        skipByteArraySizeTimesK, testRuns;

    testRuns = environment.testRuns || 1;
    domain = environment.domain || 'test_domain';
    stringLength = environment.stringlength || 10;
    byteArrayLength = environment.bytearraylength || 100;
    numRuns = environment.runs || 10000;
    timeout = environment.timeout || 3600000;
    viacc = environment.viacc || 'true';
    brokerUri = environment.brokerUri || 'tcp://localhost:1883';
    bounceProxyBaseUrl = environment.bounceProxyBaseUrl || 'http://localhost:8080';
    cchost = environment.cchost || 'localhost';
    ccport = environment.ccport || 4242;

    if (environment.skipByteArraySizeTimesK !== undefined) {
        skipByteArraySizeTimesK = environment.skipByteArraySizeTimesK;
    } else {
        skipByteArraySizeTimesK = false;
    }

    return {
        stringLength           : stringLength,
        byteArrayLength        : byteArrayLength,
        testRuns               : testRuns,
        numRuns                : numRuns,
        timeout                : timeout,
        domain                 : domain,
        brokerUri              : brokerUri,
        viacc                  : viacc,
        cchost                 : cchost,
        ccport                 : ccport,
        bounceProxyBaseUrl     : bounceProxyBaseUrl,
        skipByteArraySizeTimesK: skipByteArraySizeTimesK
    };
};

module.exports = PerformanceUtilities;
