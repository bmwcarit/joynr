/*jslint node: true, nomen: true */

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

var requirejsConfig = {
    nodeRequire : require,
    baseUrl : __dirname,
    paths : {
        "JsonParser" : "lib/JsonParser",
        "uuid" : "lib/uuid-annotated",

        "joynr/system/ConsoleAppender" : "joynr/system/ConsoleAppenderNode",
        "joynr/security/PlatformSecurityManager" : "joynr/security/PlatformSecurityManagerNode",
        "global/LocalStorage" : "global/LocalStorageNode",
        "global/XMLHttpRequest" : "global/XMLHttpRequestNode",
        "atmosphere" : "lib/atmosphereNode",
        "log4javascriptDependency" : "lib/log4javascriptNode",
        "global/WebSocket" : "global/WebSocketNode",
        "global/Mqtt" : "global/Mqtt",
        "global/Smrf" : "global/SmrfNode",
        "joynr/Runtime" : "joynr/Runtime.websocket.libjoynr"
    }
};

module.exports = requirejsConfig;

/* jslint nomen: false */
