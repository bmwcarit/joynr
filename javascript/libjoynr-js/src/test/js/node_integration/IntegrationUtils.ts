/* eslint no-console: "off" */
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

import * as MessagingQos from "../../../main/js/joynr/messaging/MessagingQos";
import provisioningRoot from "../../resources/joynr/provisioning/provisioning_root";
import joynr from "../../../main/js/joynr";

import childProcess from "child_process";
import path from "path";
let currentlyRunningChildCC: any;
const childReady: Record<string, any> = {};
const childStarted: Record<string, any> = {};
const childFinished: Record<string, Deferred> = {};
const child: Record<string, any> = {};
let processId = 0;
let messagingQos: MessagingQos;

export function log(msg: any, id: string): void {
    const logger: any = joynr.logging.getLogger(id);
    logger[msg.level](msg.message);
}

export function initialize(): void {
    messagingQos = new joynr.messaging.MessagingQos({
        ttl: provisioningRoot.ttl
    });
}

export function getObjectType(obj: any): string {
    if (obj === null || obj === undefined) {
        throw new Error("cannot determine the type of an undefined object");
    }
    const funcNameRegex = /function ([$\w]+)\(/;
    const results = funcNameRegex.exec(obj.constructor.toString());
    return results && results.length > 1 ? results[1] : "";
}

export function checkValueAndType(arg1: any, arg2: any): void {
    expect(arg1).toEqual(arg2);
    expect(typeof arg1).toEqual(typeof arg2);
    expect(getObjectType(arg1)).toEqual(getObjectType(arg2));
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

function shutdownChildProcessInternal(childId: number): void {
    if (currentlyRunningChildCC === childId) {
        currentlyRunningChildCC = undefined;
    }
    child[childId].kill();
    delete child[childId];
    delete childReady[childId];
    delete childStarted[childId];
    delete childFinished[childId];
}

export function initializeChildProcess(
    childName: string,
    provisioningSuffix: string,
    domain: string,
    processSpecialization?: any,
    cc?: any
): Promise<any> {
    processId++;
    const newChildId = processId;
    if (cc) {
        currentlyRunningChildCC = newChildId;
    }
    childReady[processId] = createPromise();
    childStarted[processId] = createPromise();
    childFinished[processId] = createPromise();

    // always use a different debugging port to avoid reusing a port if there are shutdown issues.
    const processConfig = {
        execArgv: process.execArgv.map((arg: string) => {
            if (arg.includes("--inspect")) {
                return `--inspect-brk=${getRandomInt(1024, 49151)}`;
            }
            return arg;
        })
    };

    const forked = childProcess.fork(
        path.join(__dirname, "../../../../.output/src/test/js/node_integration/provider", `${childName}.js`),
        [],
        processConfig
    );
    forked.on("message", msg => {
        // Handle messages from child process
        console.log(`received message: ${JSON.stringify(msg)}`);
        if (msg.type === "ready") {
            childReady[newChildId].resolve(newChildId);
        } else if (msg.type === "started") {
            childStarted[newChildId].resolve(msg.argument);
        } else if (msg.type === "finished") {
            childFinished[newChildId].resolve(true);
        } else if (msg.type === "error") {
            const childReadyPromise = childReady[newChildId];
            shutdownChildProcessInternal(newChildId);
            childReadyPromise.reject(new Error(msg.msg));
        }
    });
    child[newChildId] = forked;
    child[newChildId].send({
        type: "initialize",
        provisioningSuffix,
        domain,
        processSpecialization
    });

    return childReady[newChildId].promise;
}

export function startChildProcess(newChildId: string): Promise<void> {
    child[newChildId].send({
        type: "start"
    });
    return childStarted[newChildId].promise;
}

export function buildProxy(ProxyConstructor: any, domain: string): Promise<any> {
    if (domain === undefined) {
        throw new Error("specify domain");
    }
    return joynr.proxyBuilder.build(ProxyConstructor, {
        domain,
        messagingQos
    });
}

export async function shutdownChildProcess(childId: number): Promise<void> {
    if (!child[childId]) {
        throw new Error(`ChildProcess ${childId} not found`);
    }
    child[childId].send({
        type: "terminate"
    });

    await childFinished[childId].promise;
    await shutdownChildProcessInternal(childId);
}

export async function shutdownLibjoynr(): Promise<void> {
    await joynr.terminateAllSubscriptions();
    return joynr.shutdown();
}

export function waitALittle(time: number): Promise<void> {
    return new Promise(resolve => {
        setTimeout(resolve, time);
    });
}

export function getRandomInt(min: number, max: number): number {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}
