/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import LongTimer from "../../../../main/js/joynr/util/LongTimer";

/* not possible to set those values higher using jest, as the total timeout for
   jest.advanceTimersByTime is limited.
*/
const maxPow = 32; // make sure this is at least 31 to test cases with long timeout (> Math.pow(2, 31)-1)
const concurrentTimeouts = 10;
const testIntervals = 3;

describe("libjoynr-js.joynr.LongTimer.Timeout", () => {
    function testCallTimeout(timeout: number) {
        const timeoutSpy = jest.fn();
        LongTimer.setTimeout(timeoutSpy, timeout);
        jest.advanceTimersByTime(timeout - 1);
        expect(timeoutSpy).not.toHaveBeenCalled();
        jest.advanceTimersByTime(1);
        expect(timeoutSpy).toHaveBeenCalled();
        expect(timeoutSpy.mock.calls.length).toEqual(1);
    }

    function testCancelTimeout(timeout: number) {
        const timeoutSpy = jest.fn();
        const timeoutId = LongTimer.setTimeout(timeoutSpy, timeout);
        jest.advanceTimersByTime(timeout - 1);
        expect(timeoutSpy).not.toHaveBeenCalled();
        LongTimer.clearTimeout(timeoutId);
        jest.advanceTimersByTime(1);
        expect(timeoutSpy).not.toHaveBeenCalled();
    }

    beforeEach(() => {
        jest.clearAllMocks();
        jest.useFakeTimers();
    });

    afterEach(() => {
        jest.useRealTimers();
        jest.clearAllTimers();
    });

    it("provides a timeoutId", () => {
        const timeoutId = LongTimer.setTimeout(() => {
            // do nothing
        }, 0);
        expect(timeoutId).toBeDefined();
        LongTimer.clearTimeout(timeoutId);
    });

    it("calls timeout function at correct time", () => {
        let i;
        for (i = 1; i < maxPow; ++i) {
            testCallTimeout(Math.pow(2, i));
        }
    });

    it("calls concurrent timeouts correctly", () => {
        const spyArray = new Array(concurrentTimeouts).fill(null).map(() => jest.fn());

        // register spy[i] at i ms and check that no spy has been called
        for (let i = 1; i < concurrentTimeouts; ++i) {
            LongTimer.setTimeout(spyArray[i], i);
        }

        // check that spies have been called correctly
        for (let j = 0; j < concurrentTimeouts; ++j) {
            // check if spys have been called correctly
            for (let i = 1; i < concurrentTimeouts; ++i) {
                const count = i > j ? 0 : 1;
                expect(spyArray[i]).toHaveBeenCalledTimes(count);
            }

            // forward time 1 ms
            jest.advanceTimersByTime(1);
        }
    });

    it("cancels timeout correctly", () => {
        let i;
        for (i = 0; i < maxPow; ++i) {
            testCancelTimeout(Math.pow(2, i));
        }
    });

    it("calls target function with provided arguments", () => {
        const timeoutSpy = jest.fn();
        const arg1 = "arg1";
        const arg2 = "arg2";
        const timeout = 1000;
        LongTimer.setTimeout(timeoutSpy, timeout, arg1, arg2);
        jest.advanceTimersByTime(timeout - 1);
        expect(timeoutSpy).not.toHaveBeenCalled();
        jest.advanceTimersByTime(1);
        expect(timeoutSpy).toHaveBeenCalled();
        expect(timeoutSpy.mock.calls.length).toEqual(1);
        expect(timeoutSpy.mock.calls[0]).toEqual([arg1, arg2]);
    });
});

describe("libjoynr-js.joynr.LongTimer.Interval", () => {
    function testCallInterval(interval: number) {
        let i;
        const intervalSpy = jest.fn();
        const intervalId = LongTimer.setInterval(intervalSpy, interval);
        expect(intervalSpy).not.toHaveBeenCalled();
        for (i = 0; i < testIntervals; ++i) {
            jest.advanceTimersByTime(interval - 1);
            expect(intervalSpy.mock.calls.length).toEqual(i);
            jest.advanceTimersByTime(1);
            expect(intervalSpy.mock.calls.length).toEqual(i + 1);
            expect(intervalSpy).toHaveBeenCalled();
        }
        LongTimer.clearInterval(intervalId);
    }

    function testCancelInterval(interval: number) {
        const intervalSpy = jest.fn();
        const intervalId = LongTimer.setInterval(intervalSpy, interval);

        jest.advanceTimersByTime(interval);

        expect(intervalSpy).toHaveBeenCalled();
        expect(intervalSpy.mock.calls.length).toEqual(1);

        LongTimer.clearInterval(intervalId);
        jest.advanceTimersByTime(testIntervals * interval);

        expect(intervalSpy.mock.calls.length).toEqual(1);
    }

    beforeEach(() => {
        jest.clearAllMocks();
        jest.useFakeTimers();
    });

    afterEach(() => {
        jest.useRealTimers();
        jest.clearAllTimers();
    });

    it(`clearInterval won't throw if there's no interval`, () => {
        expect(() => LongTimer.clearInterval(17)).not.toThrow();
    });

    it("provides an intervalId", () => {
        const intervalId = LongTimer.setInterval(() => {
            // do nothing
        }, 0);
        expect(intervalId).toBeDefined();
        expect(typeof intervalId).toEqual("number");
        LongTimer.clearInterval(intervalId);
    });

    for (let i = 0; i < maxPow; i++) {
        it(`calls interval function at correct times ${i}`, () => {
            testCallInterval(Math.pow(2, i));
        });
    }

    it("calls concurrent timeouts correctly", () => {
        let i, j;

        const spyArray = new Array(concurrentTimeouts).fill(null).map(() => jest.fn());
        for (i = 1; i < concurrentTimeouts; ++i) {
            LongTimer.setTimeout(spyArray[i], i);
        }

        // check that spies have been called correctly
        for (j = 0; j < concurrentTimeouts; ++j) {
            // check if spys have been called correctly
            for (i = 1; i < concurrentTimeouts; ++i) {
                const count = i > j ? 0 : 1;
                expect(spyArray[i]).toHaveBeenCalledTimes(count);
            }

            // forward time 1 ms
            jest.advanceTimersByTime(1);
        }
    });

    for (let i = 0; i < maxPow; ++i) {
        it(`cancels timeout correctly ${i}`, () => {
            testCancelInterval(Math.pow(2, i));
        });
    }
});
