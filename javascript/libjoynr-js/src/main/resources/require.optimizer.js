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

// See for all options: https://gist.github.com/mattsahr/4190206
requirejs.config({

    keepBuildDir : true,
    baseUrl : "${project.build.directory}/classes",

    paths : {
        "atmosphere" : "lib/atmosphere",
        "log4javascriptDependency" : "lib/log4javascript_uncompressed",
        "bluebird" : "lib/bluebird",
        "mqtt" : "lib/mqtt",
        "JsonParser" : "lib/JsonParser",
        "uuid" : "lib/uuid-annotated",
        "text-encoding" : "lib/encoding"
    },
    shim : {
        "atmosphere" : {
            exports : "atmosphere"
        }
    }
});
