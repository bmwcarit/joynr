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

const testbase = require("test-base");
const prettyLog = testbase.logging.prettyLog;
const error = testbase.logging.error;
const IltUtil = require("./IltUtil.js");
const ExtendedTypeCollectionEnumerationInTypeCollection = require("../generated-javascript/joynr/interlanguagetest/namedTypeCollection2/ExtendedTypeCollectionEnumerationInTypeCollection.js");
/**
 * @name IltStringBroadcastFilter
 * @constructor
 *
 * @classdesc
 */
const IltStringBroadcastFilter = function IltStringBroadcastFilter() {
    if (!(this instanceof IltStringBroadcastFilter)) {
        return new IltStringBroadcastFilter();
    }

    Object.defineProperty(this, "filter", {
        enumerable: false,
        value: function filter(outputParameters, filterParameters) {
            prettyLog("IltStringBroadcastFilter: invoked");

            const stringOut = outputParameters.getStringOut();
            const stringArrayOut = outputParameters.getStringArrayOut();
            const enumerationOut = outputParameters.getEnumerationOut().name;
            const structWithStringArrayOut = outputParameters.getStructWithStringArrayOut();
            const structWithStringArrayArrayOut = outputParameters.getStructWithStringArrayArrayOut();

            const stringOfInterest = filterParameters.stringOfInterest;
            const stringArrayOfInterest = JSON.parse(filterParameters.stringArrayOfInterest);
            const enumerationOfInterest = JSON.parse(filterParameters.enumerationOfInterest);
            const structWithStringArrayOfInterest = JSON.parse(filterParameters.structWithStringArrayOfInterest);
            const structWithStringArrayArrayOfInterest = JSON.parse(
                filterParameters.structWithStringArrayArrayOfInterest
            );

            // check output parameter contents
            if (stringArrayOut === undefined || stringArrayOut === null || !IltUtil.checkStringArray(stringArrayOut)) {
                error("IltStringBroadcastFilter: invalid stringArrayOut value");
                error("IltStringBroadcastFilter: FAILED");
                return false;
            }
            if (
                enumerationOut === undefined ||
                enumerationOut === null ||
                enumerationOut !==
                    ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION.name
            ) {
                error("IltStringBroadcastFilter: invalid enumerationOut value");
                error("IltStringBroadcastFilter: FAILED");
                return false;
            }
            if (
                structWithStringArrayOut === undefined ||
                structWithStringArrayOut === null ||
                !IltUtil.checkStructWithStringArray(structWithStringArrayOut)
            ) {
                error("IltStringBroadcastFilter: invalid structWithStringArrayOut value");
                error("IltStringBroadcastFilter: FAILED");
                return false;
            }
            if (
                structWithStringArrayArrayOut === undefined ||
                structWithStringArrayArrayOut === null ||
                !IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOut)
            ) {
                error("IltStringBroadcastFilter: invalid structWithStringArrayArrayOut value");
                error("IltStringBroadcastFilter: FAILED");
                return false;
            }

            // check filter parameter contents
            if (
                stringArrayOfInterest === undefined ||
                stringArrayOfInterest === null ||
                !IltUtil.checkStringArray(stringArrayOfInterest)
            ) {
                error("IltStringBroadcastFilter: invalid stringArrayOfInterest filter parameter value");
                error("IltStringBroadcastFilter: FAILED");
                return false;
            }
            if (
                enumerationOfInterest === undefined ||
                enumerationOfInterest === null ||
                enumerationOfInterest !==
                    ExtendedTypeCollectionEnumerationInTypeCollection.ENUM_2_VALUE_EXTENSION_FOR_TYPECOLLECTION.name
            ) {
                error("IltStringBroadcastFilter: invalid enumerationOfInterest filter parameter value");
                error("IltStringBroadcastFilter: FAILED");
                return false;
            }
            if (
                structWithStringArrayOfInterest === undefined ||
                structWithStringArrayOfInterest === null ||
                !IltUtil.checkStructWithStringArray(structWithStringArrayOfInterest)
            ) {
                error("IltStringBroadcastFilter: invalid structWithStringArrayOfInterest filter parameter value");
                error("IltStringBroadcastFilter: FAILED");
                return false;
            }
            if (
                structWithStringArrayArrayOfInterest === undefined ||
                structWithStringArrayArrayOfInterest === null ||
                !IltUtil.checkStructWithStringArrayArray(structWithStringArrayArrayOfInterest)
            ) {
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

const joynr = require("joynr");
IltStringBroadcastFilter.prototype = new joynr.JoynrObject();
IltStringBroadcastFilter.prototype.constructor = IltStringBroadcastFilter;

module.exports = IltStringBroadcastFilter;
