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

module.exports = function(customMapping) {
    return {
        "preprocessor:browserify": [
            "factory",
            function() {
                var path = require("path");
                var karmaModulePath = process.argv[1];
                var browserify = require(path.join(karmaModulePath, "../../browserify"));

                return function(content, file, done) {
                    var joynrPath = path.join(karmaModulePath, "../../../src/main/js/");
                    var bundle = browserify()
                        .add(file.path)
                        .require(joynrPath, { expose: "joynr" });

                    if (customMapping) {
                        for (var fromModule in customMapping) {
                            var toModule = customMapping[fromModule];
                            bundle = bundle.require(toModule, { expose: fromModule });
                        }
                    }

                    bundle
                        .exclude("smrf-native-cpp.node")
                        .exclude("wscpp")
                        .exclude("bufferutil")
                        .exclude("utf-8-validate")
                        .exclude("node-persist")
                        .ignore(path.join(joynrPath, "joynr/start/WebSocketLibjoynrRuntime.js"))
                        .bundle(function(err, buffer) {
                            if (err) {
                                console.log(file, err.toString());
                                process.exit(1);
                            }
                            done(null, buffer.toString());
                        })
                        .on("error", function(err) {
                            console.log(file, err);
                            process.exit(1);
                        });
                };
            }
        ]
    };
};
