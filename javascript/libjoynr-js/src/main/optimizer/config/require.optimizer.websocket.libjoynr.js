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

({
    name: "almond",
    include: ["joynr"],
    out: "${project.build.directory}/jar-classes/joynr.websocket.js",
    wrap: {
        startFile: "${project.build.outputDirectory}/libjoynrStartFrag.js",
        endFile: "${project.build.outputDirectory}/libjoynrEndFrag.js"
    },
   // require.js runtime config file
    mainConfigFile: "${project.build.outputDirectory}/require.optimizer.js",
    optimize: "none", // use if you want to debug production version of joynrlib
    paths: {
        "almond": "${project.build.optimizerResources}/almond-0.2.5",
        "joynr/Runtime" : "joynr/Runtime.websocket.libjoynr"
    }
})