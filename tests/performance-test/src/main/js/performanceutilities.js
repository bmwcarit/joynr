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

const PerformanceUtilities = {};

const configName = process.env.configName || "config";
const config = require(`./config/${configName}`);
const fs = require("fs");

PerformanceUtilities.createByteArray = function(size, defaultValue) {
    const result = [];

    for (let i = 0; i < size; i++) {
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
    let domain,
        stringLength,
        byteArrayLength,
        timeout,
        cchost,
        ccport,
        skipByteArraySizeTimesK,
        testRuns,
        measureMemory,
        heapSnapShot,
        testType;

    const environment = process.env;

    const global = config.global;
    testRuns = global.testRuns || 100;
    domain = global.domain || "performance_test_domain";
    stringLength = global.stringLength || 10;
    byteArrayLength = global.byteArraySize || 100;
    timeout = global.timeout || 3600000;
    measureMemory = global.measureMemory || "true";
    heapSnapShot = global.heapSnapShot || "false";
    cchost = global.cc.host || "localhost";
    ccport = global.cc.port || 4242;
    testType = global.testType || "burst";

    if (environment.skipByteArraySizeTimesK !== undefined) {
        skipByteArraySizeTimesK = environment.skipByteArraySizeTimesK;
    } else {
        skipByteArraySizeTimesK = false;
    }

    return {
        stringLength,
        byteArrayLength,
        testRuns,
        timeout,
        domain,
        cchost,
        ccport,
        skipByteArraySizeTimesK,
        measureMemory,
        heapSnapShot,
        testType
    };
};

PerformanceUtilities.getRandomInt = function getRandomInt(min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
};

PerformanceUtilities.overrideRequire = function() {
    const LocalStorageMock = function() {
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

    LocalStorageMock.prototype.init = function() {
        return Promise.resolve();
    };

    LocalStorageMock.prototype.shutdown = function() {
        return Promise.resolve();
    };

    const mod = require("module");
    const req = mod.prototype.require;
    mod.prototype.require = function(md) {
        // mock localStorage
        if (md.endsWith("LocalStorageNode")) {
            return LocalStorageMock;
        }

        return req.apply(this, arguments);
    };
};

PerformanceUtilities.createPromise = function createPromise() {
    const map = {};
    map.promise = new Promise((resolve, reject) => {
        map.resolve = resolve;
        map.reject = reject;
    });
    return map;
};

PerformanceUtilities.findBenchmarks = function() {
    const benchmarks = config.benchmarks.filter(item => item.enabled === "true");

    return benchmarks;
};

PerformanceUtilities.getProvisioning = function(isProvider) {
    const useFSLogger = config.logging && config.logging.output === "fs";
    let provisioning;

    if (useFSLogger) {
        let loggingPath = isProvider ? "provider" : "proxy";
        loggingPath += process.pid;
        const level = config.logging.level || "info";
        provisioning = require("./config/provisioningFsLogger")(loggingPath, level);
    } else {
        provisioning = require("test-base").provisioning_common;
        provisioning.logging.configuration.loggers.root.level = "error";
    }

    if (config.tls) {
        provisioning.keychain = {};

        if (config.tls.certPath) {
            provisioning.keychain.tlsCert = fs.readFileSync(config.tls.certPath, "utf8");
        }
        if (config.tls.keyPath) {
            provisioning.keychain.tlsKey = fs.readFileSync(config.tls.keyPath, "utf8");
        }
        if (config.tls.caPath) {
            provisioning.keychain.tlsCa = fs.readFileSync(config.tls.caPath, "utf8");
        }
        provisioning.keychain.ownerId = config.tls.ownerId;

        provisioning.ccAddress.protocol = "wss";
        provisioning.ccAddress.port = 4243;
    }

    return provisioning;
};

let portOffset = 0;
/**
 * creates execArgv for child processes the same as parent but with incremented debug port
 * @returns {{execArgv: Array}}
 */
PerformanceUtilities.createChildProcessConfig = function() {
    const childArgs = [];

    for (const argument of process.execArgv) {
        if (argument.includes("--inspect")) {
            const split = argument.split("=");
            const newDebugPort = Number(split[1]) + ++portOffset;
            // inspect is either --inspect-brk or --inspect
            const inspect = split[0];
            childArgs.push(`${inspect}=${newDebugPort}`);
        } else {
            // keep the same arguments for the child as for the parent. e.g. expose-gc
            childArgs.push(argument);
        }
    }

    return { execArgv: childArgs };
};

module.exports = PerformanceUtilities;
