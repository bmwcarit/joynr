/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

(function(){
    var requireConfigCommon = function(){
        requirejs.config({

            baseUrl : "/test/classes",

            paths : {
                "joynr/Runtime" : "joynr/Runtime.inprocess",
                "atmosphere" : "lib/atmosphere",
                "log4javascript" : "lib/log4javascript_uncompressed",
                "bluebird" : "lib/bluebird",
                "JsonParser" : "lib/JsonParser",
                "uuid" : "lib/uuid-annotated",
                "global/jstestdriver": "../test-classes/global/jstestdriver",
                "global/WebSocket": "../test-classes/global/WebSocketMock",
                "joynr/provisioning": "../test-classes/joynr/provisioning",
                "Date" : "../test-classes/global/Date",
                "joynr/vehicle": "../test-classes/joynr/vehicle",
                "joynr/tests": "../test-classes/joynr/tests",
                "joynr/types/TestTypes": "../test-classes/joynr/types/TestTypes",
                "joynr/datatypes": "../test-classes/joynr/datatypes",
                "test/data": "../test-classes/test/data",
                "JstdConsoleAppender" : "../test-classes/logging/JstdConsoleAppender"
            },

            shim : {
                'atmosphere' : {
                    exports : 'atmosphere'
                }
            }
        });
    };

    var nodeOrAMD = false;
    // AMD support
//    if (typeof define === 'function' && define.amd) {
//        define("require.config.common", [], function() {
//            return requireConfigCommon;
//        });
//        nodeOrAMD = true;
//    }
    if (typeof exports === 'object') {
        // Support Node.js specific `module.exports` (which can be a function)
        if (typeof module !== 'undefined' && module.exports) {
            exports = module.exports = requireConfigCommon;
        }
        nodeOrAMD = true;
        module.exports = requireConfigCommon;
    }
    if (!nodeOrAMD) {
        requireConfigCommon();
    }
}());