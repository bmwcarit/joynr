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
var LongTimer = require('../../../classes/joynr/util/LongTimer');
var Typing = require('../../../classes/joynr/util/Typing');

var maxPow = 35; // make sure this is at lease 31 to test cases with long timeout (> Math.pow(2, 31)-1)
var concurrentTimeouts = 10;
var testIntervals = 10;

describe("libjoynr-js.joynr.LongTimer.Timeout", function() {

    function testCallTimeout(timeout) {
        jasmine.clock().uninstall();
        jasmine.clock().install();
        var timeoutSpy = jasmine.createSpy("timeoutSpy");
        LongTimer.setTimeout(timeoutSpy, timeout);
        jasmine.clock().tick(timeout - 1);
        expect(timeoutSpy).not.toHaveBeenCalled();
        jasmine.clock().tick(1);
        expect(timeoutSpy).toHaveBeenCalled();
        expect(timeoutSpy.calls.count()).toEqual(1);
    }

    function testCancelTimeout(timeout) {
        jasmine.clock().uninstall();
        jasmine.clock().install();
        var timeoutSpy = jasmine.createSpy("timeoutSpy");
        var timeoutId = LongTimer.setTimeout(timeoutSpy, timeout);
        jasmine.clock().tick(timeout - 1);
        expect(timeoutSpy).not.toHaveBeenCalled();
        LongTimer.clearTimeout(timeoutId);
        jasmine.clock().tick(1);
        expect(timeoutSpy).not.toHaveBeenCalled();
    }

    beforeEach(function(done) {
        jasmine.clock().install();
        done();
    });

    afterEach(function(done) {
        jasmine.clock().uninstall();
        done();
    });

    it("provides a timeoutId", function(done) {
        var timeoutId = LongTimer.setTimeout(function() {}, 0);
        expect(timeoutId).toBeDefined();
        expect(Typing.getObjectType(timeoutId)).toEqual("Number");
        done();
    });

    it("calls timeout function at correct time", function(done) {
        var i;
        for (i = 0; i < maxPow; ++i) {
            testCallTimeout(Math.pow(2, i));
        }
        done();
    });

    it("calls concurrent timeouts correctly", function(done) {
        var i, j, spy;

        var spyArray = [];
        for (i = 1; i <= concurrentTimeouts; ++i) {
            spyArray.push("timeout" + i);
        }
        spy = jasmine.createSpyObj("spy", spyArray);

        // register spy[i] at i ms and check that no spy has been called
        for (i = 1; i <= concurrentTimeouts; ++i) {
            LongTimer.setTimeout(spy["timeout" + i], i);
        }

        // check that spies have been called correctly
        for (j = 0; j <= concurrentTimeouts; ++j) {
            // check if spys have been called correctly
            for (i = 1; i <= concurrentTimeouts; ++i) {
                var e = expect(spy["timeout" + i]);
                if (i > j) {
                    e = e.not;
                }
                e.toHaveBeenCalled();
            }

            // forward time 1 ms
            jasmine.clock().tick(1);
        }
        done();
    });

    it("cancels timeout correctly", function(done) {
        var i;
        for (i = 0; i < maxPow; ++i) {
            testCancelTimeout(Math.pow(2, i));
        }
        done();
    });

    it("calls target function with provided arguments", function(done) {
        var timeoutSpy = jasmine.createSpy("timeoutSpy");
        var arg1 = "arg1";
        var arg2 = "arg2";
        var timeout = 1000;
        LongTimer.setTimeout(timeoutSpy, timeout, arg1, arg2);
        jasmine.clock().tick(timeout - 1);
        expect(timeoutSpy).not.toHaveBeenCalled();
        jasmine.clock().tick(1);
        expect(timeoutSpy).toHaveBeenCalled();
        expect(timeoutSpy.calls.count()).toEqual(1);
        expect(timeoutSpy.calls.mostRecent().args[0]).toEqual(arg1);
        expect(timeoutSpy.calls.mostRecent().args[1]).toEqual(arg2);
        done();
    });
});

describe("libjoynr-js.joynr.LongTimer.Interval", function() {

    function testCallInterval(interval) {
        var i;
        jasmine.clock().uninstall();
        jasmine.clock().install();
        var intervalSpy = jasmine.createSpy("intervalSpy");
        LongTimer.setInterval(intervalSpy, interval);
        expect(intervalSpy).not.toHaveBeenCalled();
        for (i = 0; i < testIntervals; ++i) {
            jasmine.clock().tick(interval - 1);
            expect(intervalSpy.calls.count()).toEqual(i);
            jasmine.clock().tick(1);
            expect(intervalSpy.calls.count()).toEqual(i + 1);
            expect(intervalSpy).toHaveBeenCalled();
        }
    }

    function testCancelInterval(interval) {
        var i;
        jasmine.clock().uninstall();
        jasmine.clock().install();
        var intervalSpy = jasmine.createSpy("intervalSpy");
        var intervalId = LongTimer.setInterval(intervalSpy, interval);

        jasmine.clock().tick(interval);

        expect(intervalSpy).toHaveBeenCalled();
        expect(intervalSpy.calls.count()).toEqual(1);

        LongTimer.clearInterval(intervalId);
        jasmine.clock().tick(testIntervals * interval);

        expect(intervalSpy.calls.count()).toEqual(1);
    }

    beforeEach(function(done) {
        jasmine.clock().install();
        done();
    });

    afterEach(function(done) {
        jasmine.clock().uninstall();
        done();
    });

    it("provides an intervalId", function(done) {
        var intervalId = LongTimer.setInterval(function() {}, 0);
        expect(intervalId).toBeDefined();
        expect(Typing.getObjectType(intervalId)).toEqual("Number");
        done();
    });

    it("calls interval function at correct times", function(done) {
        var i;
        for (i = 0; i < maxPow; ++i) {
            testCallInterval(Math.pow(2, i));
        }
        done();
    });

    it("calls concurrent timeouts correctly", function(done) {
        var i, j, spy;

        var spyArray = [];
        for (i = 1; i <= concurrentTimeouts; ++i) {
            spyArray.push("timeout" + i);
        }
        spy = jasmine.createSpyObj("spy", spyArray);

        // register spy[i] at i ms and check that no spy has been called
        for (i = 1; i <= concurrentTimeouts; ++i) {
            LongTimer.setTimeout(spy["timeout" + i], i);
        }

        // check that spies have been called correctly
        for (j = 0; j <= concurrentTimeouts; ++j) {
            // check if spys have been called correctly
            for (i = 1; i <= concurrentTimeouts; ++i) {
                var e = expect(spy["timeout" + i]);
                if (i > j) {
                    e = e.not;
                }
                e.toHaveBeenCalled();
            }

            // forward time 1 ms
            jasmine.clock().tick(1);
        }
        done();
    });

    it("cancells timeout correctly", function(done) {
        var i;
        for (i = 0; i < maxPow; ++i) {
            testCancelInterval(Math.pow(2, i));
        }
        done();
    });
});
