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
    include: ["intertab/comm/BrowserInterTabService"],
    out: "${project.build.outputDirectory}/lib/BrowserInterTabService.js",
    wrap: {
        startFile: "${project.build.outputDirectory}/BITServiceStartFrag.js",
        endFile: "${project.build.outputDirectory}/BITServiceEndFrag.js"
    },

    // require.js runtime config file
    mainConfigFile: "${project.build.outputDirectory}/require.config.js",

    optimize: "none", // use if you want to debug production version

    wrapShim: true,

    paths: {
        "almond": "${project.build.optimizerResources}/almond-0.2.5",
//        "joynr/runtime" : "joynr/runtime.intertab.libjoynr"
    }
})