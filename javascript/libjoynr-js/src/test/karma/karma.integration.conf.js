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
var isDocker = true;

module.exports = function(config) {
    var flags = ["--headless", "--disable-gpu", "--remote-debugging-port=9222"];
    if (isDocker) flags.push("--no-sandbox");
    config.set({
        customLaunchers: {
            ChromeCustom: {
                base: "Chrome",
                flags: flags
            }
        },
        plugins: [
            // Karma will require() these plugins
            "karma-jasmine",
            "karma-chrome-launcher",
            "karma-junit-reporter",
            "karma-verbose-reporter",
            require("./karma.preprocessor.browserify")()
        ],

        // base path that will be used to resolve all patterns (eg. files, exclude)
        // libjoynr/src in this case
        basePath: "../..",

        // frameworks to use
        // available frameworks: https://npmjs.org/browse/keyword/karma-adapter
        frameworks: ["jasmine"],

        // list of files / patterns to load in the browser
        // in other words load nothing but the InProcessRuntimeTest.js
        files: [
            { pattern: "main/js/lib/*.js", included: false },
            { pattern: "*.js", included: false },
            { pattern: "test/js/global/*.js", included: false },
            { pattern: "main/js/global/*.js", included: false },
            { pattern: "main/js/joynr.js", included: false },
            { pattern: "main/js/libjoynr-deps.js", included: false },
            { pattern: "main/js/joynr/**/*.js", included: false },
            { pattern: "test/js/test/**/*.js", included: false },
            { pattern: "test/js/joynr/provisioning/*.js", included: false },
            { pattern: "test/js/joynr/start/InProcessRuntimeTest.js", included: true }
            //{pattern: 'test-classes/integration/HttpMessagingTest.js', included: false},
        ],

        // list of files to exclude
        exclude: ["test/js/global/**/*Test.js"],

        // preprocess matching files before serving them to the browser
        // available preprocessors: https://npmjs.org/browse/keyword/karma-preprocessor
        preprocessors: {
            "test/js/joynr/start/InProcessRuntimeTest.js": ["browserify"]
        },

        // test results reporter to use
        // possible values: 'dots', 'progress'
        // available reporters: https://npmjs.org/browse/keyword/karma-reporter
        //reporters: ['progress', 'junit'],
        reporters: ["verbose", "junit"],

        // web server port
        port: 9876,

        // enable / disable colors in the output (reporters and logs)
        colors: true,

        // level of logging
        // possible values: config.LOG_DISABLE || config.LOG_ERROR || config.LOG_WARN || config.LOG_INFO || config.LOG_DEBUG
        logLevel: config.LOG_INFO,
        //logLevel: config.LOG_DEBUG,

        // enable / disable watching file and executing tests whenever any file changes
        autoWatch: true,

        // start these browsers
        // available browser launchers: https://npmjs.org/browse/keyword/karma-launcher
        // browsers: ["ChromeHeadless"],
        browsers: ["ChromeCustom"],
        // browsers: ["Chrome"],
        // browsers: ['PhantomJS'],

        // Continuous Integration mode
        // if true, Karma captures browsers, runs the tests and exits
        singleRun: false,

        // Concurrency level
        // how many browser should be started simultanous
        concurrency: Infinity,

        // outputDir is already located in 'target'
        junitReporter: {
            outputDir: "../target/test-results",
            outputFile: "Test-karma-Integration.xml",
            suite: "",
            useBrowserName: false,
            nameFormatter: undefined,
            classNameFormatter: undefined,
            properties: {}
        }
    });
};
