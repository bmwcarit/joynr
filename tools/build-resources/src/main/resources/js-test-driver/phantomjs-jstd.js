/*
 * The MIT License
 *
 * Copyright (c) 2013 Larry Myers <larry@larrymyers.com>
 * Copyright (c) 2017 BMW Car IT GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// Source: https://github.com/larrymyers/js-test-driver-phantomjs/blob/master/phantomjs-jstd.js
// MIT License: https://github.com/larrymyers/js-test-driver-phantomjs/blob/master/LICENSE

localStorage.clear();

var page = require('webpage').create();
var system = require('system');
var url = system.args[1];
var captureAttempts = 0;
var captured = false;
var locked = false;

var log = function(str) {
    var dt = new Date();
    console.log(dt.toString() + ': ' + str);
};

var pageLoaded = function(status) {
    log('Finished loading ' + url + ' with status: ' + status);

    var runnerFrame = page.evaluate(function() {
        return document.getElementById('runner');
    });

    if (!runnerFrame) {
        locked = false;
        setTimeout(capture, 1000);
    } else {
        captured = true;
    }
};

var capture = function() {
    if (captureAttempts === 5) {
        log('Failed to capture JSTD after ' + captureAttempts + ' attempts.');
        phantom.exit();
    }

    if (captured || locked) {
        return;
    }

    captureAttempts += 1;
    locked = true;

    log('Attempting (' + captureAttempts + ') to load: ' + url);
    page.open(url, pageLoaded);
};

capture();
