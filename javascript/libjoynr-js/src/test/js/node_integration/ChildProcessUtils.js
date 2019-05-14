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
const ChildProcessUtils = {};

ChildProcessUtils.postReady = function() {
    process.send({
        type: "ready"
    });
};

ChildProcessUtils.postError = errorMsg => {
    process.send({
        type: "error",
        msg: errorMsg
    });
};

ChildProcessUtils.postStarted = function(argument) {
    const object = {
        type: "started"
    };
    if (argument !== undefined) {
        object.argument = argument;
    }
    process.send(object);
};

ChildProcessUtils.postFinished = function() {
    process.send({
        type: "finished"
    });
};

ChildProcessUtils.registerHandlers = function(initializeTest, startTest, terminateTest) {
    let runtime;
    const handler = async function(msg) {
        console.log(JSON.stringify(msg));
        if (msg.type === "initialize") {
            if (!initializeTest) {
                throw new Error("cannot initialize test, child does not define an initializeTest method");
            }
            try {
                const providedRuntime = await initializeTest(
                    msg.provisioningSuffix,
                    msg.domain,
                    msg.processSpecialization
                );
                runtime = providedRuntime;
                ChildProcessUtils.postReady();
            } catch (e) {
                ChildProcessUtils.postError(`failed to initialize child process: ${e}`);
            }
        } else if (msg.type === "start") {
            if (!startTest) {
                throw new Error("cannot start test, child does not define a startTest method");
            }
            startTest().then(argument => {
                ChildProcessUtils.postStarted(argument);
            });
        } else if (msg.type === "terminate") {
            if (!terminateTest) {
                throw new Error("cannot terminate test, child does not define a terminate method");
            }
            terminateTest()
                .then(runtime.shutdown)
                .then(ChildProcessUtils.postFinished);
        }
    };
    process.on("message", handler);
};

module.exports = ChildProcessUtils;
