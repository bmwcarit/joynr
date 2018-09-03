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

const provisioning = require("./provisioning_root");
provisioning.brokerUri = "tcp://127.0.0.1:1883";
provisioning.bounceProxyBaseUrl = "http://127.0.0.1:8080";
provisioning.bounceProxyUrl = `${provisioning.bounceProxyBaseUrl}/bounceproxy/`;

provisioning.internalMessagingQos = {
    ttl: provisioning.ttl
};

provisioning.logging = {
    configuration: {
        name: "test config",
        appenders: {
            Console: {
                name: "STDOUT",
                PatternLayout: {
                    pattern: "%m%n"
                }
            }
        },
        loggers: {
            root: {
                level: "error",
                AppenderRef: {
                    ref: "STDOUT"
                }
            }
        }
    }
};


provisioning.persistency = {
  routingTable: false, capabilities: false, publications: false
};


module.exports = provisioning;
