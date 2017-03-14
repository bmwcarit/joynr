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

var preProcessorList = [];

// Skip code instrumentation, if karma is called with '--debug' option
if (process.argv.indexOf("--debug") == -1) {
    preProcessorList = [ 'coverage' ];
}

module.exports = function(config) {
  config.set({
      plugins: [
            // Karma will require() these plugins
            'karma-jasmine',
            'karma-chrome-launcher',
            'karma-phantomjs-launcher',
            'karma-requirejs',
            'karma-junit-reporter',
            'karma-verbose-reporter',
            'karma-coverage'
    ],

    // base path that will be used to resolve all patterns (eg. files, exclude)
    basePath: '../../../target',


    // frameworks to use
    // available frameworks: https://npmjs.org/browse/keyword/karma-adapter
    frameworks: ['jasmine', 'requirejs'],


    // list of files / patterns to load in the browser
    // list of files / patterns to load in the browser
    files: [
            {pattern: 'classes/lib/*.js', included: false},
            {pattern: 'classes/lib/encoding.js', included: true},
            {pattern: 'test-classes/global/*.js', included: false},
            {pattern: 'classes/global/*.js', included: false},
            {pattern: 'classes/joynr.js', included: false},
            {pattern: 'classes/joynr/**/*.js', included: false},
            {pattern: 'test-classes/test/**/*.js', included: false},
            {pattern: 'test-classes/joynr/**/*.js', included: false},
            'test-classes/test-unit.js'
    ],


    // list of files to exclude
    exclude: [
            'test-classes/joynr/start/InProcessRuntimeTest.js'
    ],


    // preprocess matching files before serving them to the browser
    // available preprocessors: https://npmjs.org/browse/keyword/karma-preprocessor
    preprocessors: {
      'classes/joynr.js' : preProcessorList,
      'classes/global/*.js' : preProcessorList,
      'classes/joynr/**/*.js' : preProcessorList,
      'classes/joynr.js' : preProcessorList
    },


    // test results reporter to use
    // possible values: 'dots', 'progress'
    // available reporters: https://npmjs.org/browse/keyword/karma-reporter
    //reporters: ['progress', 'junit'],
    reporters: ['verbose', 'junit', 'coverage' ],


    // web server port
    port: 9876,


    // enable / disable colors in the output (reporters and logs)
    colors: true,


    // level of logging
    // possible values: config.LOG_DISABLE || config.LOG_ERROR || config.LOG_WARN || config.LOG_INFO || config.LOG_DEBUG
    logLevel: config.LOG_INFO,


    // enable / disable watching file and executing tests whenever any file changes
    autoWatch: true,


    // start these browsers
    // available browser launchers: https://npmjs.org/browse/keyword/karma-launcher
    //browsers: ['Chrome'],
    browsers: ['PhantomJS'],


    // Continuous Integration mode
    // if true, Karma captures browsers, runs the tests and exits
    singleRun: false,

    // Concurrency level
    // how many browser should be started simultanous
    concurrency: Infinity,

    // outputDir is already located in 'target'
    junitReporter: {
      outputDir: 'test-results',
      outputFile: 'TestUnit.xml',
      suite: '',
      useBrowserName: false,
      nameFormatter: undefined,
      classNameFormatter: undefined,
      properties: {}
    }
  })
};
