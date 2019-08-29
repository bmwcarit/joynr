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

const fs = require("fs");
const child_process = require("child_process");
const version = require("../src/main/js/package.json").version;
const fileTemplate = fs.readFileSync(`${__dirname}/buildSignatureTemplate.ts`, "utf8");
const gitSha = child_process
    .execSync("git rev-parse HEAD")
    .toString()
    .trim();

const signatureString = `io.joynr.javascript.libjoynr-js-${version}-r${gitSha}${new Date().toISOString()}`;

const buildSignature = fileTemplate.replace("buildSignature", signatureString);
fs.writeFileSync(`${__dirname}/../src/main/js/joynr/buildSignature.ts`, buildSignature);
