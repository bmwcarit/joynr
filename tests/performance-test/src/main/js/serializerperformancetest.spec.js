/*jslint node: true */

/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

var PerformanceUtilities = require("./performanceutilities.js");
var btoa = require('btoa');
var atob = require('atob');
var uuid = require('uuid')
var numWarmups = 1000
var numRuns = 1000

describe("Serializer Performance Test", function() {
    var toMicro = function(highResTime) {
        return highResTime[0] * 1000000 + highResTime[1] / 1000;
    }

    var executeTest = function(testName, testRunner) {
        for(var i=0 ; i < numWarmups ; i++) {
            testRunner()
        }

        var startTimeHighRes = process.hrtime();
        for(var i=0 ; i < numRuns ; i++) {
            testRunner();
        }
        var endTimeHighRes = process.hrtime();

        var elapsedTimeMs = (toMicro(endTimeHighRes) - toMicro(startTimeHighRes)) / 1000;

        console.log(testName + ". average: " + (elapsedTimeMs / numRuns).toFixed(5) +
                    " ms, " + "total: " + elapsedTimeMs.toFixed(3) + " ms");
    };

    var runNonBase64_encode = function(byteArraySize, byteArrayInitValue) {
        var byteArray = PerformanceUtilities.createByteArray(byteArraySize, byteArrayInitValue);

        var testProcedure = function() {
             JSON.stringify(byteArray)
        }

        executeTest(nameTest("Non-Base64", "encode", byteArraySize, byteArrayInitValue), testProcedure)
    }

    var runNonBase64_decode = function(byteArraySize, byteArrayInitValue) {
        var jsonString = JSON.stringify(PerformanceUtilities.createByteArray(byteArraySize, byteArrayInitValue));

        var testProcedure = function() {
            JSON.parse(jsonString)
        }

        executeTest(nameTest("Non-Base64", "decode", byteArraySize, byteArrayInitValue), testProcedure)
    }

    var runBase64_encode = function(byteArraySize, byteArrayInitValue) {
        var byteArray = PerformanceUtilities.createByteArray(byteArraySize, byteArrayInitValue);

        var testProcedure = function() {
            var base64ByteArray = btoa(byteArray);
            JSON.stringify(base64ByteArray);
        }

        executeTest(nameTest("Base64", "encode", byteArraySize, byteArrayInitValue), testProcedure)
    }

    var runBase64_decode = function(byteArraySize, byteArrayInitValue) {
        var jsonString = JSON.stringify(btoa(PerformanceUtilities.createByteArray(byteArraySize, byteArrayInitValue)));

        var testProcedure = function() {
            var byteArray = JSON.parse(jsonString)
            atob(byteArray)
        }

        executeTest(nameTest("Base64", "decode", byteArraySize, byteArrayInitValue), testProcedure)
    }

    var nameTest = function(baseMode, encodeMode, byteArraySize, byteArrayInitValue) {
        return baseMode + ", " + encodeMode + ", " + byteArraySize + " bytes, " + "array content: " + byteArrayInitValue
    }

    it("Non-Base64 - encode", function() {
        console.log("Runs: " + numRuns + ", Warmups: " + numWarmups)
        runNonBase64_encode(1000, 123)
        runNonBase64_encode(1000, 1)
        runNonBase64_encode(10000, 123)
        runNonBase64_encode(10000, 1)
        runNonBase64_encode(100000, 123)
        runNonBase64_encode(100000, 1)
    });

    it("Base64 - encode", function() {
        console.log("Runs: " + numRuns + ", Warmups: " + numWarmups)
        runBase64_encode(1000, 123)
        runBase64_encode(1000, 1)
        runBase64_encode(10000, 123)
        runBase64_encode(10000, 1)
        runBase64_encode(100000, 123)
        runBase64_encode(100000, 1)
    });

    it("Non-Base64 - decode", function() {
        console.log("Runs: " + numRuns + ", Warmups: " + numWarmups)
        runNonBase64_decode(1000, 123)
        runNonBase64_decode(1000, 1)
        runNonBase64_decode(10000, 123)
        runNonBase64_decode(10000, 1)
        runNonBase64_decode(100000, 123)
        runNonBase64_decode(100000, 1)
    });

    it("Base64 - decode", function() {
        console.log("Runs: " + numRuns + ", Warmups: " + numWarmups)
        runBase64_decode(1000, 123)
        runBase64_decode(1000, 1)
        runBase64_decode(10000, 123)
        runBase64_decode(10000, 1)
        runBase64_decode(100000, 123)
        runBase64_decode(100000, 1)
    });

    // Measure how passing a simple replacer function affects the performance.
    it("Serialize With Replacement", function() {
        replacerFunction = function replacerFunction(key, src) {
            // Perform a simple operation on the data.
            return src.testKey.trim();
        };

        var testProcedure = function() {
            var testType = { testKey : uuid() };
            JSON.stringify(testType, replacerFunction)
        }

        executeTest("Serializer with replacement", testProcedure);
    });

    it("Serialize Without Replacement", function() {
        var testProcedure = function() {
            var testType = { testKey : uuid() };
            JSON.stringify(testType)
        }

        executeTest("Serializer without replacement", testProcedure);
    });
});
