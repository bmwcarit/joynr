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

import joynr from "../../../main/js/joynr";

export function postReady(): void {
    // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
    process.send!({
        type: "ready"
    });
}

export function postError(errorMsg: string): void {
    // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
    process.send!({
        type: "error",
        msg: errorMsg
    });
}

export function postStarted(argument: any): void {
    const object: any = {
        type: "started"
    };
    if (argument !== undefined) {
        object.argument = argument;
    }
    // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
    process.send!(object);
}

export function postFinished(): void {
    // eslint-disable-next-line @typescript-eslint/no-non-null-assertion
    process.send!({
        type: "finished"
    });
}

export function registerHandlers(initializeTest: Function, startTest: Function, terminateTest: Function): void {
    const handler = async (msg: {
        type: string;
        provisioningSuffix: any;
        domain: any;
        processSpecialization: any;
    }): Promise<void> => {
        if (msg.type === "initialize") {
            if (!initializeTest) {
                throw new Error("cannot initialize test, child does not define an initializeTest method");
            }
            try {
                await initializeTest(msg.provisioningSuffix, msg.domain, msg.processSpecialization);
                postReady();
            } catch (e) {
                postError(`failed to initialize child process: ${e}`);
            }
        } else if (msg.type === "start") {
            if (!startTest) {
                throw new Error("cannot start test, child does not define a startTest method");
            }
            const argument = await startTest();
            postStarted(argument);
        } else if (msg.type === "terminate") {
            if (!terminateTest) {
                throw new Error("cannot terminate test, child does not define a terminate method");
            }
            terminateTest()
                .then(joynr.shutdown)
                .then(postFinished);
        }
    };
    process.on("message", handler);
}
