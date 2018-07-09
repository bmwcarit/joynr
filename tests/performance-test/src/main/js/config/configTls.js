/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

const baseConfig = require("./config");
baseConfig.global.cc.port = "4243";
baseConfig.tls = {
    certPath: "/data/ssl-data/certs/client.cert.pem",
    keyPath: "/data/ssl-data/private/client.key.pem",
    caPath: "/data/ssl-data/certs/ca.cert.pem",
    ownerId: "client"
};

for (let i = 0; i < baseConfig.benchmarks.length; i++) {
    baseConfig.benchmarks[i].numRuns /= 12;
}

module.exports = baseConfig;
