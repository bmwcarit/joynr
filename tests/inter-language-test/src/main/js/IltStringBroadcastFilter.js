/*jslint node: true */

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

(function(undefined) {
    var testbase = require("test-base");
    var log = testbase.logging.log;
    var prettyLog = testbase.logging.prettyLog;
    var error = testbase.logging.error;
    var IltUtil = require("./IltUtil.js");
    var ExtendedTypeCollectionEnumerationInTypeCollection = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedTypeCollectionEnumerationInTypeCollection.js");
    /**
     * @name IltStringBroadcastFilter
     * @constructor
     *
     * @classdesc
     */
    var IltStringBroadcastFilter = function IltStringBroadcastFilter() {
        if (!(this instanceof IltStringBroadcastFilter)) {
            return new IltStringBroadcastFilter();
        }

        Object.defineProperty(this, 'filter', {
            enumerable: false,
            value: function filter(outputParameters, filterParameters) {
                prettyLog("IltStringBroadcastFilter: invoked");

                var stringOut = outputParameters.getStringOut();
                var stringArrayOut = outputParameters.getStringArrayOut();
                var enumerationOut = outputParameters.getEnumerationOut().name;
                var structWithStringArrayOut = outputParameters.getStructWithStringArrayOut();
                var structWithStringArrayArrayOut = outputParameters.getStructWithStringArrayArrayOut();

                var stringOfInterest = filterParameters.stringOfInterest;
                var stringArrayOfInterest = JSON.parse(filterParameters.stringArrayOfInterest);
                var enumerationOfInterest = JSON.parse(filterParameters.enumerationOfInterest);
                var structWithStringArrayOfInterest = JSON.parse(filterParameters.structWithStringArrayOfInterest);
                var structWithStringArrayArrayOfInterest = JSON.parse(filterParameters.structWithStringArrayArrayOfInterest);

                // check output parameter contents
                if (stringArrayOut === undefined || stringArrayOut === null || !IltUtil.checkStringArray(stringArrayOut)) {
                    error("IltStringBroadcastFilter: invalid stringArrayOut value");
                    error("IltStringBroadcastFilter: FAILED");
                    return false;
                }
                if (enumerationOut === undefined || enumerationOut === null || enumerationOut != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION.name) {
                    error("IltStringBroadcastFilter: invalid enumerationOut value");
                    error("IltStringBroadcastFilter: FAILED");
                    return false;
                }
                if (structWithStringArrayOut === undefined || structWithStringArrayOut == null || !IltUtil.checkStructWithStringArray(structWithStringArrayOut)) {
                    error("IltStringBroadcastFilter: invalid structWithStringArrayOut value");
                    error("IltStringBroadcastFilter: FAILED");
                    return false;
                }
                if (structWithStringArrayArrayOut === undefined || structWithStringArrayArrayOut === null || !IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOut)) {
                    error("IltStringBroadcastFilter: invalid structWithStringArrayArrayOut value");
                    error("IltStringBroadcastFilter: FAILED");
                    return false;
                }

                // check filter parameter contents
                if (stringArrayOfInterest === undefined || stringArrayOfInterest === null || !IltUtil.checkStringArray(stringArrayOfInterest)) {
                    error("IltStringBroadcastFilter: invalid stringArrayOfInterest filter parameter value");
                    error("IltStringBroadcastFilter: FAILED");
                    return false;
                }
                if (enumerationOfInterest === undefined || enumerationOfInterest === null || enumerationOfInterest != ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION.name) {
                    error("IltStringBroadcastFilter: invalid enumerationOfInterest filter parameter value");
                    error("IltStringBroadcastFilter: FAILED");
                    return false;
                }
                if (structWithStringArrayOfInterest === undefined || structWithStringArrayOfInterest === null || !IltUtil.checkStructWithStringArray(structWithStringArrayOfInterest)) {
                    error("IltStringBroadcastFilter: invalid structWithStringArrayOfInterest filter parameter value");
                    error("IltStringBroadcastFilter: FAILED");
                    return false;
                }
                if (structWithStringArrayArrayOfInterest === undefined || structWithStringArrayArrayOfInterest === null || !IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOfInterest)) {
                    error("IltStringBroadcastFilter: invalid structWithStringArrayArrayOfInterest filter parameter value");
                    error("IltStringBroadcastFilter: FAILED");
                    return false;
                }

                // decision for publication is made based on stringOfInterest
                if (stringOfInterest === stringOut) {
                    prettyLog("IltStringBroadcastFilter: OK - publication should be sent");
                    return true;
                } else {
                    prettyLog("IltStringBroadcastFilter: OK - publication should NOT Be sent");
                    return false;
                }
            }
        });
    };

    // the following variable is just needed to bypass JSLint check
    // 'typeof' is the only way working without exceptions
    var exportsCheck = typeof exports;
    // AMD support
    if (typeof define === 'function' && define.amd) {
        define(["joynr"], function (joynr) {
            IltStringBroadcastFilter.prototype = new joynr.JoynrObject();
            IltStringBroadcastFilter.prototype.constructor = IltStringBroadcastFilter;
            joynr.addType("joynr.interlanguagetest.IltStringBroadcastFilter", IltStringBroadcastFilter);
            return IltStringBroadcastFilter;
        });
    } else if (exportsCheck !== 'undefined') {
        if ((module !== undefined) && module.exports) {
            exports = module.exports = IltStringBroadcastFilter;
        } else {
            // support CommonJS module 1.1.1 spec (`exports` cannot be a function)
            exports.IltStringBroadcastFilter = IltStringBroadcastFilter;
        }
        var joynr = requirejs("joynr");
        IltStringBroadcastFilter.prototype = new joynr.JoynrObject();
        IltStringBroadcastFilter.prototype.constructor = IltStringBroadcastFilter;

        joynr.addType("joynr.interlanguagetest.IltStringBroadcastFilter", IltStringBroadcastFilter);
    } else {
        IltStringBroadcastFilter.prototype = new window.joynr.JoynrObject();
        IltStringBroadcastFilter.prototype.constructor = IltStringBroadcastFilter;
        window.joynr.addType("joynr.interlanguagetest.IltStringBroadcastFilter", IltStringBroadcastFilter);
        window.IltStringBroadcastFilter = IltStringBroadcastFilter;
    }
}());
