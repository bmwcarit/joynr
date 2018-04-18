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

let fs = require("fs");
let fileTemplate = fs.readFileSync(__dirname + "/buildSignatureTemplate.js", "utf8");
let gitSha = require("child_process")
    .execSync("git rev-parse HEAD")
    .toString()
    .trim();

let signatureString =
    "io.joynr.javascript.libjoynr-js-" +
    require("../src/main/js/package.json").version +
    "-r" +
    gitSha +
    new Date().toISOString();

let buildSignature = fileTemplate.replace("buildSignature", signatureString);
fs.writeFileSync(__dirname + "/../src/main/js/joynr/buildSignature.js", buildSignature);
