/*jslint node: true es5: true */

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

var log = require("./logging.js").log;

var runInteractiveConsole = function(radioProvider, onDone) {
    var readline = require('readline');
    var rl = readline.createInterface(process.stdin, process.stdout);
    rl.setPrompt('>> ');
    var MODES = {
        HELP : {
            value : "h",
            description : "help",
            options : {}
        },
        QUIT : {
            value : "q",
            description : "quit",
            options : {}
        },
        SHUFFLE : {
            value : "shuffle",
            description : "shuffle current station",
            options : {}
        }
    };

    var showHelp = require("./console_common.js");
    rl.on('line', function(line) {
        var input = line.trim().split(' ');
        switch (input[0]) {
            case MODES.HELP.value:
                showHelp(MODES);
                break;
            case MODES.QUIT.value:
                rl.close();
                break;
            case MODES.SHUFFLE.value:
                radioProvider.shuffleStations.callOperation([], []);
                break;
            case '':
                break;
            default:
                log('unknown input: ' + input);
                break;
        }
        rl.prompt();
    });

    rl.on('close', function() {
        if (onDone) {
            onDone();
        }
        process.exit(0);
    });

    showHelp(MODES);
    rl.prompt();
};

if (process.argv.length !== 3) {
    log("please pass a domain as argument");
    process.exit(0);
}
var domain = process.argv[2];
log("domain: " + domain);

var joynr = require("joynr");
var provisioning = require("./provisioning_common.js");
var RadioProvider = require("../generated/js/joynr/vehicle/RadioProvider.js");
var MyRadioProvider = require("./MyRadioProvider.js");
joynr.load(provisioning).then(function(loadedJoynr) {
    log("joynr started");
    joynr = loadedJoynr;

    var providerQos = new joynr.types.ProviderQos({
        customParameters : [],
        providerVersion : 1,
        priority : Date.now(),
        scope : joynr.types.ProviderScope.GLOBAL,
        onChangeSubscriptions : true
    });

    var radioProvider = joynr.providerBuilder.build(
        RadioProvider,
        MyRadioProvider.implementation);
    MyRadioProvider.setProvider(radioProvider);

    joynr.capabilities.registerCapability("", domain, radioProvider, providerQos).then(function() {
        log("provider registered successfully");
        runInteractiveConsole(radioProvider);
        return null;
    }).catch(function(error) {
        log("error registering provider: " + error.toString());
    });
    return loadedJoynr;
}).catch(function(error){
    throw error;
});