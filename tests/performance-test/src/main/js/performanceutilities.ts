/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

const configName = process.env.configName || "config";
const config = require(`./config/${configName}`);
import fs from "fs";
import child_process from "child_process";

export function createByteArray(size: number, defaultValue: any): any[] {
    const result = [];

    for (let i = 0; i < size; i++) {
        result.push(defaultValue);
    }

    return result;
}

export function createString(length: number, defaultChar: string): string {
    // Add one because the character is inserted between the array elements.
    return String(new Array(length + 1).join(defaultChar));
}

export function createRandomNumber(max: number): number {
    return Math.floor(Math.random() * (max + 1));
}

export function forceGC(): void {
    if (global.gc) {
        global.gc();
    } else {
        console.error("no gc hook! (Start node with --expose-gc  -> use npm run startconsumer)");
    }
}

/**
 * Reads command line arguments from environment. If an argument is not
 * available, a default value will be used.
 */
export function getCommandLineOptionsOrDefaults() {
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
}

export function getCcPiD(): string {
    let pid: string;
    try {
        const execResult = child_process.execSync("ps -ef | grep cluster-controller | grep -v grep").toString();

        if (execResult.length === 0) {
            throw new Error("did not find a cc pid. -> Is the clustercontroller running? ");
        }
        pid = execResult.replace(/\s+/g, " ").split(" ")[1];
    } catch (e) {
        // ps -ef is not available -> assume busybox
        const execResult = child_process.execSync("ps | grep cluster-controller | grep -v grep").toString();
        pid = execResult
            .replace(/\s+/g, " ")
            .trim()
            .split(" ")[0];
        // trim is necessary because ps sometimes has a blank first
    }

    console.log(`cluster-controller pid is: ${pid}`);
    const pidNr = Number(pid);
    if (isNaN(pidNr)) {
        throw new Error(`cluster-controller pid is not a number: ${pid}`);
    }

    return pid;
}

export function getRandomInt(min: number, max: number) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

export interface Deferred {
    promise: Promise<any>;
    resolve: (...args: any[]) => void;
    reject: (...args: any[]) => void;
}

export function createPromise(): Deferred {
    const map: Deferred = {} as any;
    map.promise = new Promise((resolve, reject) => {
        map.resolve = resolve;
        map.reject = reject;
    });

    return map;
}

export function findBenchmarks() {
    return config.benchmarks.filter((item: { enabled: string }) => item.enabled === "true");
}

export function getProvisioning(isProvider: boolean) {
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
    if (config.keychain) {
        provisioning.keychain = config.keychain;

        provisioning.ccAddress.protocol = "wss";
        provisioning.ccAddress.port = 4243;
    } else if (config.tls) {
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
}

let portOffset = 0;

/**
 * creates execArgv for child processes the same as parent but with incremented debug port
 * @returns {{execArgv: Array}}
 */
export function createChildProcessConfig(): { execArgv: any[] } {
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
    // compilation for child_process
    childArgs.push("-r");
    childArgs.push("ts-node/register");

    return { execArgv: childArgs };
}
