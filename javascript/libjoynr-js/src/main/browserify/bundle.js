/*
 * #%L
 * %%
 * Copyright (C) 2018 BMW Car IT GmbH
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

const fs = require("fs");
const browserify = require("browserify");

const inputFile = "../js/joynr.js";
const outputFile = "joynr.bundle.js";

const bundler = browserify(inputFile, {
    browserField: false,
    commondir: false,
    bare: true,
    builtins: false,
    standalone: "joynr.bundle"
});

return new Promise((resolve, reject) => {
    const ws = fs.createWriteStream(outputFile);
    ws.on("finish", resolve);
    ws.on("error", reject);

    ["start/InProcessRuntime"].map(
        module => {
            bundler.ignore(`./${module}.js`, { basedir: "../js/joynr" });
        }
    );

    bundler
        .external("wscpp")
        .external("smrf-native")
        .external("ws")
        .external("bluebird")
        .transform("uglifyify", {
            global: true,
            mangle: false,
            output: {
                comments: /@license|@preserve|@author|@copyright|^!/
            }
        })
        .bundle()
        .pipe(ws);
}).catch(err => {
    console.error("Bundling failed due to", err);
    process.exit(1);
});
