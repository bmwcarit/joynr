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

requirejs.config({

    baseUrl : "/test/classes",

    paths : {
        "atmosphere" : "lib/atmosphere",
        "log4javascript" : "lib/log4javascript",
        "bluebird" : "lib/bluebird",
        "JsonParser" : "lib/JsonParser",
        "uuid" : "lib/uuid-annotated",
        "global/WebSocket": "../test-classes/global/WebSocketMock",
        "Date" : "../test-classes/global/Date",
        "joynr/vehicle": "../test-classes/joynr/vehicle",
        "test/data": "../test-classes/test/data"

    },

    shim : {
        'atmosphere' : {
            exports : 'atmosphere'
        }
    }

});
