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
// max value for timeout, see
// http://stackoverflow.com/questions/3468607/why-does-settimeout-break-for-large-millisecond-delay-values
/**
 * Implementation for long (>2^31-1 ms) timer functions that only allows timeouts or intervals
 * of 2^31-1 ms (because of SignedInt32 representation)
 *
 * @name LongTimer
 * @class
 */
var LongTimer = {};

LongTimer.maxTime = Math.pow(2, 31) - 1;
LongTimer.idPrefix = "lt";

var highestTimeoutId = 0;
LongTimer.timeoutMap = {};

function timeoutPortion(timeoutId) {
    // retrieve timeout object
    var timeoutObj = LongTimer.timeoutMap[timeoutId];

    // timeout has been cancelled
    if (timeoutObj === undefined) {
        return;
    }

    // if timeout elapsed, remove timeout object and call function
    if (timeoutObj.remainingTimeout === 0) {
        delete LongTimer.timeoutMap[timeoutId];
        timeoutObj.func.apply(this, timeoutObj.args);
        return;
    }

    // recalculate remaining timeout and start next portion of timeout
    var timeToWait = Math.min(LongTimer.maxTime, timeoutObj.remainingTimeout);
    timeoutObj.remainingTimeout -= timeToWait;
    timeoutObj.currentTimeout = setTimeout(timeoutPortion, timeToWait, timeoutId);
}

/**
 * Implementation for long (>2^31-1 ms) window.setTimeout only allows intervals of 2^31-1 ms
 * (because of SignedInt32 representation)
 *
 * @function LongTimer#setTimeout
 * @param {Function}
 *            func
 * @param {Number}
 *            timeout
 * @returns {String|Object} timeout identifier
 * @throws {Error}
 *             if parameters are nullable or not of documented type
 */
LongTimer.setTimeout = function(func, timeout) {
    if (timeout <= LongTimer.maxTime) {
        return setTimeout.apply(null, arguments);
    }

    // get next timeout id and prefix it to avoid possible collisions in environents
    // where setTimeout returns a number (e.g. in browsers).
    highestTimeoutId++;
    var timeoutId = LongTimer.idPrefix + highestTimeoutId;

    // put timeout object into map
    LongTimer.timeoutMap[timeoutId] = {
        func: func,
        remainingTimeout: timeout,
        args: Array.prototype.slice.call(arguments, 2)
        // get the arbitrary arguments
    };

    // start timeout
    if (timeout === 0) {
        setTimeout(timeoutPortion, 0, timeoutId);
    } else {
        timeoutPortion(timeoutId);
    }
    // return id to
    return timeoutId;
};

/**
 * Clears the timeout
 *
 * @function LongTimer#clearTimeout
 * @param timeoutId
 *            the timeout id given by setTimeout
 * @throws {Error}
 *             if parameters are nullable or not of documented type
 */
LongTimer.clearTimeout = function(timeoutId) {
    // retrieve timeout object
    var timeoutObj = LongTimer.timeoutMap[timeoutId];

    // timeout has run out, been cancelled already or was less than maxTime
    if (timeoutObj === undefined) {
        return clearTimeout(timeoutId);
    }

    // stop javascript interval and remove timeout object
    clearTimeout(timeoutObj.currentTimeout);
    LongTimer.timeoutMap[timeoutId] = undefined;
};

var highestIntervalId = -1;
var intervalMap = {};

/**
 * Implementation for long (>2^31-1 ms) window.setInterval only allows intervals of 2^31-1 ms
 * (because of SignedInt32 representation)
 *
 * @function LongTimer#setInterval
 * @param {Function}
 *            func
 * @param {Number}
 *            interval
 * @returns {Number} interval identifier
 * @throws {Error}
 *             if parameters are nullable or not of documented type
 */
LongTimer.setInterval = function(func, interval) {
    // get next interval id
    var intervalId = ++highestIntervalId;

    function intervalPortion() {
        // retrieve timeout object
        var intervalObj = intervalMap[intervalId];

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
        var timeToWait = Math.min(LongTimer.maxTime, intervalObj.remainingInterval);
        intervalObj.remainingInterval -= timeToWait;
        intervalObj.currentTimeout = setTimeout(intervalPortion, timeToWait);
    }

    // put interval object into map
    intervalMap[intervalId] = {
        func: func,
        remainingInterval: interval,
        interval: interval
    };

    // start interval
    intervalPortion();

    // return id to
    return intervalId;
};

/**
 * Clears the interval
 *
 * @function LongTimer#clearInterval
 * @param intervalId
 *            the interval id given by setInterval
 * @throws {Error}
 *             if parameters are nullable or not of documented type
 */
LongTimer.clearInterval = function(intervalId) {
    // retrieve interval object
    var interval = intervalMap[intervalId];

    // interval has run out or been cancelled already
    if (interval === undefined) {
        return;
    }

    // stop javascript timeout and remove interval object
    clearTimeout(interval.currentTimeout);
    delete intervalMap[intervalId];
};

module.exports = LongTimer;
