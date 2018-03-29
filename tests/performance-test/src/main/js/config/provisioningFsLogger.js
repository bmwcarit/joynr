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

var fs = require("fs");

function createProvisioningWithFsLogger(path, level) {
    function FSAppender() {
        // don't write anything in here. It won't get called. LoggingManager overrides this ...
    }

    let outputStream = fs.createWriteStream(path);

    FSAppender.prototype.append = function(loggingEvent) {
        let formattedMessage = loggingEvent.messages.join(", ");
        outputStream.write(formattedMessage);
    };

    var provisioning = {};
    provisioning.ccAddress = {
        protocol: "ws",
        host: "localhost",
        port: 4242,
        path: ""
    };

    provisioning.logging = {
        configuration: {
            appenders: {
                appender: [
                    {
                        type: "Custom",
                        name: "CUSTOM",
                        PatternLayout: {
                            pattern: "%m"
                        }
                    }
                ]
            },
            loggers: {
                root: {
                    level: level,
                    AppenderRef: [
                        {
                            ref: "CUSTOM"
                        }
                    ]
                }
            }
        },
        appenderClasses: {
            Custom: FSAppender
        }
    };
    return provisioning;
}

module.exports = createProvisioningWithFsLogger;
