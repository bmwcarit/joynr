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
import child_process from "child_process";
import path from "path";
import { Deferred } from "./performanceutilities";
import * as PerformanceUtilities from "./performanceutilities";

class ChildProcessController {
    protected file: string;
    public process!: child_process.ChildProcess;
    protected ready!: Deferred;
    protected measurementPromise!: Deferred;
    protected prepareBenchmarkPromise!: Deferred;
    protected executeBenchmarkPromise!: Deferred;
    protected subsciptionFinishedPromise!: Deferred;
    protected broadcastsReceivedPromise!: Deferred;
    public constructor(type: string, public prepareBenchmark: Function, public executeBenchmark: Function) {
        this.file = type === "provider" ? "providerChildProcess.ts" : "proxyChildProcess.ts";
        this.initialize = this.initialize.bind(this);
    }

    public initialize() {
        const config: any = PerformanceUtilities.createChildProcessConfig();
        config.env = Object.create(process.env);
        const fileLocation = path.join(__dirname, this.file);
        this.process = child_process.fork(fileLocation, [], config);
        this.ready = PerformanceUtilities.createPromise();

        this.process.on("message", (msg: { msg: any; data: any }) => {
            switch (msg.msg) {
                case "initialized":
                    this.ready.resolve();
                    break;
                case "gotMeasurement":
                    this.measurementPromise.resolve(msg.data);
                    break;
                case "prepareBenchmarkFinished":
                    this.prepareBenchmarkPromise.resolve();
                    break;
                case "executeBenchmarkFinished":
                    this.executeBenchmarkPromise.resolve();
                    break;
                case "subscriptionFinished":
                    this.subsciptionFinishedPromise.resolve();
                    break;
                case "receivedBroadcasts":
                    this.broadcastsReceivedPromise.resolve();
                    break;
                default:
                    throw new Error(`unknown MessageType${JSON.stringify(msg)}`);
            }
        });

        return this.ready.promise;
    }

    public shutdown() {
        this.process.send({ msg: "terminate" });
    }

    public startMeasurement() {
        this.process.send({ msg: "startMeasurement" });
    }

    public stopMeasurement() {
        this.process.send({ msg: "stopMeasurement" });
        this.measurementPromise = PerformanceUtilities.createPromise();
        return this.measurementPromise.promise;
    }

    public prepareForBroadcast(benchmarkConfig: { numRuns: number }) {
        this.process.send({ msg: "subscribeBroadcast", amount: benchmarkConfig.numRuns });
        this.subsciptionFinishedPromise = PerformanceUtilities.createPromise();
        this.broadcastsReceivedPromise = PerformanceUtilities.createPromise();
        return this.subsciptionFinishedPromise.promise;
    }
}

export = ChildProcessController;
