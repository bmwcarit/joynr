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
    var JoynrTestRequire = function(requireOrig) {
        /**
         * Convenience function to define jasmine test cases which use requirejs to
         * asynchronously load dependencies. Adds an asynchronous jasmine test case which waits
         * until the passed callback function has completed. Thus, it is possible to define the
         * actual test cases within this callback function.  Same signature as requirejs.
         */
        this.joynrTestRequire = function(name, deps, callback, errback, optional){
            var callback_wrapper = function(callback, name) {
                var tests_defined = false;
                var callback_modified = callback;
                if (typeof callback === "function") {
                    describe("All tests in " + name + " ...", function() {
                        it("are defined", function() {
                            waitsFor(function() {
                                return tests_defined;
                            });
                            runs(function() {
                                expect(tests_defined).toBeTruthy();
                            });
                        });
                    });

                    callback_modified = function() {
                        callback.apply(this, arguments);
                        tests_defined = true;
                    };
                }
                return callback_modified;
            }
            requireOrig(deps, callback_wrapper(callback, name), errback, optional);

        };
    }

    /* disable requirejs support for joynrTestRequire, as it shall be attached to the window object
     * in case of jstestdriver test execution
     */
    // AMD support
    /*if (typeof define === 'function' && define.amd) {
        define("joynrTestRequire", [], function() {
            return JoynrTestRequire;
        });
    } else */ if (typeof exports !== 'undefined') {
        // Support Node.js specific `module.exports` (which can be a function)
        if (typeof module !== 'undefined' && module.exports) {
            exports = module.exports = JoynrTestRequire;
        }
        // But always support CommonJS module 1.1.1 spec (`exports` cannot be a function)
        exports.JoynrTestRequire = JoynrTestRequire;
    } else {
        window.JoynrTestRequire = JoynrTestRequire;
    }
}());