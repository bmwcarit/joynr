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
    var requireConfigSystem = function(requireConfigCommonFct){
        if (requireConfigCommonFct !== undefined) {
            requireConfigCommonFct();
        }

        requirejs.config({
            paths : {
                "joynr" : "../jar-classes/joynr",
                "joynr/start/settings/defaultInterTabSettings" : "../classes/joynr/start/settings/defaultInterTabSettings",
                "integration": "../test-classes/integration",
                "joynr/vehicle" : "../test-classes/joynr/vehicle",
                "joynr/datatypes" : "../test-classes/joynr/datatypes"
            }
        });
    };

    var nodeOrAMD = false;
    // AMD support
//    if (typeof define === 'function' && define.amd) {
//        define("require.config.intertab", ["require.config.common"], function(requireConfigCommonFct) {
//            return requireConfigSystem(requireConfigCommonFct);
//        });
//        nodeOrAMD = true;
//    }
    if (typeof exports === 'object') {
        // Support Node.js specific `module.exports` (which can be a function)
        nodeOrAMD = true;
        var exportFct = function() {
            requireConfigSystem(require("require.config.common"));
        };

        if (typeof module !== 'undefined' && module.exports) {
            exports = module.exports = exportFct;
        }
        module.exports = exportFct;
    }
    if (!nodeOrAMD) {
        //it is assumed that the common config has already been loaded
        requireConfigSystem();
    }
}());