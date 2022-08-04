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

interface Mapping<T> {
    [key: string]: T;
}

// max value for timeout, see
// http://stackoverflow.com/questions/3468607/why-does-settimeout-break-for-large-millisecond-delay-values
/**
 * Implementation for long (>2^31-1 ms) timer functions that only allows timeouts or intervals
 * of 2^31-1 ms (because of SignedInt32 representation)
 */
class LongTimer {
    public readonly maxTime: number = Math.pow(2, 31) - 1;
    public readonly idPrefix: string = "lt";
    private timeoutMap: Mapping<any> = {};
    private highestTimeoutId = 0;
    private highestIntervalId = -1;
    private intervalMap: Mapping<any> = {};

    /**
     * Implementation for long (>2^31-1 ms) window.setTimeout only allows intervals of 2^31-1 ms
     * (because of SignedInt32 representation)
     *
     * @param func timout handler
     * @param timeout in ms for function to be called
     * @param args extra parameters for func
     * @returns timeout identifier
     * @throws {Error} if parameters are nullable or not of documented type
     */
    public setTimeout(func: Function, timeout: number, ...args: any[]): string | number {
        if (timeout <= this.maxTime) {
            return setTimeout(func, timeout, ...args);
        }

        // get next timeout id and prefix it to avoid possible collisions in environments
        // where setTimeout returns a number (e.g. in browsers).
        this.highestTimeoutId++;
        const timeoutId: string = this.idPrefix + this.highestTimeoutId;

        // put timeout object into map
        this.timeoutMap[timeoutId] = {
            func,
            remainingTimeout: timeout,
            args
        };

        this.timeoutPortion(timeoutId);
        return timeoutId;
    }

    private timeoutPortion = (timeoutId: string): void => {
        // retrieve timeout object
        const timeoutObj = this.timeoutMap[timeoutId];

        // timeout has been cancelled
        if (timeoutObj === undefined) {
            return;
        }

        // if timeout elapsed, remove timeout object and call function
        if (timeoutObj.remainingTimeout === 0) {
            delete this.timeoutMap[timeoutId];
            timeoutObj.func.apply(this, timeoutObj.args);
            return;
        }

        // recalculate remaining timeout and start next portion of timeout
        const timeToWait = Math.min(this.maxTime, timeoutObj.remainingTimeout);
        timeoutObj.remainingTimeout -= timeToWait;
        timeoutObj.currentTimeout = setTimeout(this.timeoutPortion, timeToWait, timeoutId);
    };

    /**
     * Clears the timeout
     *
     * @param timeoutId the timeout id given by setTimeout
     * @throws {Error} if parameters are nullable or not of documented type
     */
    public clearTimeout(timeoutId: string | number): void {
        // retrieve timeout object
        const timeoutObj = this.timeoutMap[timeoutId];

        // timeout has run out, been cancelled already or was less than maxTime
        if (timeoutObj === undefined) {
            return clearTimeout(timeoutId as number);
        }

        // stop javascript interval and remove timeout object
        clearTimeout(timeoutObj.currentTimeout);
        this.timeoutMap[timeoutId] = undefined;
    }

    /**
     * Implementation for long (>2^31-1 ms) window.setInterval only allows intervals of 2^31-1 ms
     * (because of SignedInt32 representation)
     *
     * @param func
     * @param interval
     * @returns interval identifier
     * @throws {Error} if parameters are nullable or not of documented type
     */
    public setInterval(func: Function, interval: number): number {
        // get next interval id
        const intervalId = ++this.highestIntervalId;

        const intervalPortion = (): void => {
            // retrieve timeout object
            const intervalObj = this.intervalMap[intervalId];

            // interval has been cancelled
            if (intervalObj === undefined) {
                return;
            }

            // if interval elapsed, call function and reset remaining interval time
            if (intervalObj.remainingInterval === 0) {
                intervalObj.func();
                intervalObj.remainingInterval = intervalObj.interval;
            }

            // recalculate remaining timeout and start next portion of interval
            const timeToWait = Math.min(this.maxTime, intervalObj.remainingInterval);
            intervalObj.remainingInterval -= timeToWait;
            intervalObj.currentTimeout = setTimeout(intervalPortion, timeToWait);
        };

        // put interval object into map
        this.intervalMap[intervalId] = {
            func,
            remainingInterval: interval,
            interval
        };

        // start interval
        intervalPortion();

        // return id to
        return intervalId;
    }

    /**
     * Clears the interval
     *
     * @param intervalId the interval id given by setInterval
     * @throws {Error} if parameters are nullable or not of documented type
     */
    public clearInterval(intervalId: number): void {
        // retrieve interval object
        const interval = this.intervalMap[intervalId];

        // interval has run out or been cancelled already
        if (interval === undefined) {
            return;
        }

        // stop javascript timeout and remove interval object
        clearTimeout(interval.currentTimeout);
        delete this.intervalMap[intervalId];
    }
}
type longTimer = LongTimer;
const longTimer = new LongTimer();
export = longTimer;
