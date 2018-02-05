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

var configName = process.env.configName || "config";
var config = require("./config/" + configName);

PerformanceUtilities.createByteArray = function(size, defaultValue) {
    var result = [];

    for (var i = 0; i < size; i++) {
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

PerformanceUtilities.forceGC = function() {
    if (global.gc) {
        global.gc();
    } else {
        console.error("no gc hook! (Start node with --expose-gc  -> use npm run startconsumer)");
    }
};

/**
 * Reads command line arguments from environment. If an argument is not
 * available, a default value will be used.
 */
PerformanceUtilities.getCommandLineOptionsOrDefaults = function() {
    var domain,
        stringLength,
        byteArrayLength,
        timeout,
        cchost,
        ccport,
        skipByteArraySizeTimesK,
        testRuns,
        measureMemory,
        testType;

    var environment = process.env;

    var global = config.global;
    testRuns = global.testRuns || 100;
    domain = global.domain || "performance_test_domain";
    stringLength = global.stringLength || 10;
    byteArrayLength = global.byteArraySize || 100;
    timeout = global.timeout || 3600000;
    measureMemory = global.measureMemory || "true";
    cchost = global.cc.host || "localhost";
    ccport = global.cc.port || 4242;
    testType = global.testType || "burst";

    if (environment.skipByteArraySizeTimesK !== undefined) {
        skipByteArraySizeTimesK = environment.skipByteArraySizeTimesK;
    } else {
        skipByteArraySizeTimesK = false;
    }

    return {
        stringLength: stringLength,
        byteArrayLength: byteArrayLength,
        testRuns: testRuns,
        timeout: timeout,
        domain: domain,
        cchost: cchost,
        ccport: ccport,
        skipByteArraySizeTimesK: skipByteArraySizeTimesK,
        measureMemory: measureMemory,
        testType
    };
};

PerformanceUtilities.getRandomInt = function getRandomInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
};

PerformanceUtilities.overrideRequire = function() {
    var LocalStorageMock = function() {
        this.map = {};
    };

    LocalStorageMock.prototype.setItem = function(key, value) {
        this.map[key] = value;
    };

    LocalStorageMock.prototype.getItem = function(key) {
        return this.map[key];
    };

    LocalStorageMock.prototype.removeItem = function(key) {
        delete this.map[key];
    };

    LocalStorageMock.prototype.clear = function() {
        this.map = {};
    };

    var mod = require("module");
    var req = mod.prototype.require;
    mod.prototype.require = function(md) {
        // mock localStorage
        if (md.endsWith("LocalStorageNode")) {
            return LocalStorageMock;
        }

        return req.apply(this, arguments);
    };
};

PerformanceUtilities.createPromise = function createPromise() {
    var map = {};
    map.promise = new Promise(function(resolve, reject) {
        map.resolve = resolve;
        map.reject = reject;
    });
    return map;
};

PerformanceUtilities.findBenchmarks = function() {
    var benchmarks = config.benchmarks.filter(item => item.enabled === "true");

    return benchmarks;
};

module.exports = PerformanceUtilities;
