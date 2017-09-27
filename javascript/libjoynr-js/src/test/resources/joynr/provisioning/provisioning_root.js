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

(function(){
    var setupProvisionedData = function(provisioning){
        provisioning.ttl = ${joynr.provisioning.testTtl};

        provisioning.mqtt = {
            qosLevel : 0
        };
        provisioning.logging = {
                configuration : {
                    name : "test config",
                    appenders : {
                        Console : {
                            name : "STDOUT",
                            PatternLayout : {
                                pattern : "%m%n"
                            }
                        }
                    },
                    loggers : {
                        root : {
                            level : "debug",
                            AppenderRef : {
                                ref : "STDOUT"
                            }
                        }
                    }
                }
        };

        return provisioning;
    };

    // AMD support
    if (typeof define === 'function' && define.amd) {
        define("joynr/provisioning/provisioning_root", [], function() {
            return setupProvisionedData({});
        });
    } else if (typeof exports !== 'undefined') {
        module.exports = setupProvisionedData({});
    } else {
        window.joynr = window.joynr || {};
        window.joynr.provisioning = window.joynr.provisioning || {};
        setupProvisionedData(window.joynr.provisioning);
    }
}());
