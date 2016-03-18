/*jslint node: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

/**
 * Reads command line arguments from process.env. If an argument is not
 * available, a default value will be used.
 */
PerformanceUtilities.getCommandLineOptionsOrDefaults = function(environment) {
    if(environment.domain != undefined) {
    	domain = process.env.domain;
    } else {
    	domain = "test_domain";
    }

    if(environment.stringlength != undefined) {
        stringLength = process.env.stringlength;
    } else {
    	stringLength = 10;
    }

    if(environment.bytearraylength != undefined) {
        byteArrayLength = process.env.bytearraylength;
    } else {
    	byteArrayLength = 100;
    }

    if(environment.runs != undefined) {
        numRuns = process.env.runs;
    } else {
    	numRuns = 10000;
    }

    if(environment.timeout != undefined) {
    	timeout = process.env.timeout;
    } else {
    	timeout = 3600000;
    }

    return {
    	'stringLength' : stringLength,
    	'byteArrayLength' : byteArrayLength,
    	'numRuns' : numRuns,
    	'timeout' : timeout,
    	'domain' : domain
    };
};

module.exports = PerformanceUtilities;
