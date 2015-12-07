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

define([
    "joynr/util/LongTimer",
    "joynr/util/Typing"
], function(LongTimer, Typing) {

    var maxPow = 35; // make sure this is at lease 31 to test cases with long timeout (> Math.pow(2, 31)-1)
    var concurrentTimeouts = 10;
    var testIntervals = 10;

    describe("libjoynr-js.joynr.LongTimer.Timeout", function() {

        function testCallTimeout(timeout) {
            jasmine.Clock.reset();
            var timeoutSpy = jasmine.createSpy("timeoutSpy");
            LongTimer.setTimeout(timeoutSpy, timeout);
            jasmine.Clock.tick(timeout - 1);
            expect(timeoutSpy).not.toHaveBeenCalled();
            jasmine.Clock.tick(1);
            expect(timeoutSpy).toHaveBeenCalled();
            expect(timeoutSpy.calls.length).toEqual(1);
        }

        function testCancelTimeout(timeout) {
            jasmine.Clock.reset();
            var timeoutSpy = jasmine.createSpy("timeoutSpy");
            var timeoutId = LongTimer.setTimeout(timeoutSpy, timeout);
            jasmine.Clock.tick(timeout - 1);
            expect(timeoutSpy).not.toHaveBeenCalled();
            LongTimer.clearTimeout(timeoutId);
            jasmine.Clock.tick(1);
            expect(timeoutSpy).not.toHaveBeenCalled();
        }

        beforeEach(function() {
            jasmine.Clock.useMock();
            jasmine.Clock.reset();
        });

        it("throws on null, undefined or wrongly typed parameters", function() {
            expect(function() {
                LongTimer.setTimeout(undefined, 123);
            }).toThrow();
            expect(function() {
                LongTimer.setTimeout(null, 123);
            }).toThrow();
            expect(function() {
                LongTimer.setTimeout(function() {}, undefined);
            }).toThrow();
            expect(function() {
                LongTimer.setTimeout(function() {}, null);
            }).toThrow();
            expect(function() {
                LongTimer.setTimeout(123, 123);
            }).toThrow();
            expect(function() {
                LongTimer.setTimeout(function() {}, function() {});
            }).toThrow();
            expect(function() {
                LongTimer.setTimeout(function() {}, 0);
            }).not.toThrow();
            expect(function() {
                LongTimer.clearTimeout();
            }).toThrow();
            expect(function() {
                LongTimer.clearTimeout(undefined);
            }).toThrow();
            expect(function() {
                LongTimer.clearTimeout(null);
            }).toThrow();
            expect(function() {
                LongTimer.clearTimeout("asdf");
            }).toThrow();
            expect(function() {
                LongTimer.clearTimeout(0);
            }).not.toThrow();
        });

        it("provides a timeoutId", function() {
            var timeoutId = LongTimer.setTimeout(function() {}, 0);
            expect(timeoutId).toBeDefined();
            expect(Typing.getObjectType(timeoutId)).toEqual("Number");
        });

        it("calls timeout function at correct time", function() {
            var i;
            for (i = 0; i < maxPow; ++i) {
                testCallTimeout(Math.pow(2, i));
            }
        });

        it("calls concurrent timeouts correctly", function() {
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
                jasmine.Clock.tick(1);
            }
        });

        it("cancels timeout correctly", function() {
            var i;
            for (i = 0; i < maxPow; ++i) {
                testCancelTimeout(Math.pow(2, i));
            }
        });

        it("calls target function with provided arguments", function() {
            jasmine.Clock.reset();
            var timeoutSpy = jasmine.createSpy("timeoutSpy");
            var arg1 = "arg1";
            var arg2 = "arg2";
            var timeout = 1000;
            LongTimer.setTimeout(timeoutSpy, timeout, arg1, arg2);
            jasmine.Clock.tick(timeout - 1);
            expect(timeoutSpy).not.toHaveBeenCalled();
            jasmine.Clock.tick(1);
            expect(timeoutSpy).toHaveBeenCalled();
            expect(timeoutSpy.calls.length).toEqual(1);
            expect(timeoutSpy.mostRecentCall.args[0]).toEqual(arg1);
            expect(timeoutSpy.mostRecentCall.args[1]).toEqual(arg2);
        });
    });

    describe("libjoynr-js.joynr.LongTimer.Interval", function() {

        function testCallInterval(interval) {
            var i;
            jasmine.Clock.reset();
            var intervalSpy = jasmine.createSpy("intervalSpy");
            LongTimer.setInterval(intervalSpy, interval);
            expect(intervalSpy).not.toHaveBeenCalled();
            for (i = 0; i < testIntervals; ++i) {
                jasmine.Clock.tick(interval - 1);
                expect(intervalSpy.calls.length).toEqual(i);
                jasmine.Clock.tick(1);
                expect(intervalSpy.calls.length).toEqual(i + 1);
                expect(intervalSpy).toHaveBeenCalled();
            }
        }

        function testCancelInterval(interval) {
            var i;
            jasmine.Clock.reset();
            var intervalSpy = jasmine.createSpy("intervalSpy");
            var intervalId = LongTimer.setInterval(intervalSpy, interval);

            jasmine.Clock.tick(interval);

            expect(intervalSpy).toHaveBeenCalled();
            expect(intervalSpy.calls.length).toEqual(1);

            LongTimer.clearInterval(intervalId);
            jasmine.Clock.tick(testIntervals * interval);

            expect(intervalSpy.calls.length).toEqual(1);
        }

        beforeEach(function() {
            jasmine.Clock.useMock();
            jasmine.Clock.reset();
        });

        it("throws on null, undefined or wrongly typed parameters", function() {
            expect(function() {
                LongTimer.setInterval(undefined, 123);
            }).toThrow();
            expect(function() {
                LongTimer.setInterval(null, 123);
            }).toThrow();
            expect(function() {
                LongTimer.setInterval(function() {}, undefined);
            }).toThrow();
            expect(function() {
                LongTimer.setInterval(function() {}, null);
            }).toThrow();
            expect(function() {
                LongTimer.setInterval(123, 123);
            }).toThrow();
            expect(function() {
                LongTimer.setInterval(function() {}, function() {});
            }).toThrow();
            expect(function() {
                LongTimer.setInterval(function() {}, 0);
            }).not.toThrow();
            expect(function() {
                LongTimer.clearInterval();
            }).toThrow();
            expect(function() {
                LongTimer.clearInterval(undefined);
            }).toThrow();
            expect(function() {
                LongTimer.clearInterval(null);
            }).toThrow();
            expect(function() {
                LongTimer.clearInterval("asdf");
            }).toThrow();
            expect(function() {
                LongTimer.clearInterval(0);
            }).not.toThrow();
        });

        it("provides an intervalId", function() {
            var intervalId = LongTimer.setInterval(function() {}, 0);
            expect(intervalId).toBeDefined();
            expect(Typing.getObjectType(intervalId)).toEqual("Number");
        });

        it("calls interval function at correct times", function() {
            var i;
            for (i = 0; i < maxPow; ++i) {
                testCallInterval(Math.pow(2, i));
            }
        });

        it("calls concurrent timeouts correctly", function() {
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
                jasmine.Clock.tick(1);
            }
        });

        it("cancells timeout correctly", function() {
            var i;
            for (i = 0; i < maxPow; ++i) {
                testCancelInterval(Math.pow(2, i));
            }
        });
    });
});
