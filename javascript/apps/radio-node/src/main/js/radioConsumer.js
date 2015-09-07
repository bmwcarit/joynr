/*jslint node: true  es5: true */

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
var prettyLog = require("./logging.js").prettyLog;

var runDemo = function(radioProxy, onDone) {
    var RadioStation = require("../generated/js/joynr/vehicle/RadioStation");
    var Country = require("../generated/js/joynr/vehicle/Country");

    prettyLog("ATTRIBUTE GET: currentStation...");
    radioProxy.currentStation.get().catch(function(error) {
        prettyLog("ATTRIBUTE GET: currentStation failed: " + error);
    }).then(function(value) {
        prettyLog("ATTRIBUTE GET: currentStation returned: " + JSON.stringify(value));
        prettyLog("RPC: radioProxy.addFavoriteStation(radioStation)...");
        return radioProxy.addFavoriteStation(
            {
                newFavoriteStation : new RadioStation({
                    name : "runDemoFavoriteStation",
                    trafficService : true,
                    country : Country.GERMANY
                })
            }        
        );
    }).catch(function(error) {
        prettyLog("RPC: radioProxy.addFavoriteStation(radioStation) failed: " + error);
    }).then(function() {
        prettyLog("RPC: radioProxy.addFavoriteStation(radioStation) returned");
        prettyLog("radioProxy.shuffleStations()...");
        return radioProxy.shuffleStations();
    }).catch(function(error) {
        prettyLog("RPC: radioProxy.shuffleStations() failed: " + error);
    }).then(function(value) {
        prettyLog("RPC: radioProxy.shuffleStations() returned");
        prettyLog("ATTRIBUTE GET: currentStation after shuffle...");
        return radioProxy.currentStation.get();
    }).catch(function(error) {
        prettyLog("ATTRIBUTE GET: currentStation failed: " + error);
    }).then(function(value) {
        prettyLog("ATTRIBUTE GET: currentStation returned: " + JSON.stringify(value));
    });
};

var subscription = require("./subscription.js");
var isOnSubscription = {};
isOnSubscription.subscriptions = {};
isOnSubscription.setJoynr = function(joynr) {
    this.joynr = joynr;
};
isOnSubscription.subscribe =
        function(radioProxy, onDone) {
            if (!this.joynr) {
                log("you first have to initialize me with setJoynr! Aborting.");
            } else {
                var subscriptionQosOnChange = new this.joynr.proxy.OnChangeSubscriptionQos({
                    minInterval : 50
                });

                var onPublicationIsOn = function(value) {
                    prettyLog("Received update of isOn: " + value);
                };

                subscription.subscribeAttribute(
                        radioProxy,
                        "isOn",
                        "onChange",
                        subscriptionQosOnChange,
                        onPublicationIsOn,
                        this.subscriptions,
                        onDone);
            }
        };
isOnSubscription.unsubscribe = function(radioProxy, onDone) {
    subscription.unsubscribeAttributeSubscriptions(radioProxy, "isOn", this.subscriptions, onDone);
};

var runInteractiveConsole =
        function(radioProxy, onDone) {
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
                SET_IS_ON : {
                    value : "setIsOn",
                    description : "set value for isOn",
                    options : {
                        TRUE : "true",
                        FALSE : "false"
                    }
                },
                GET_IS_ON : {
                    value : "getIsOn",
                    description : "get value for isOn",
                    options : {}
                },
                ADD_FAVORITE_STATION : {
                    value : "addFavStation",
                    description : "add a Favorite Station",
                    options : {
                        NAME : "name"
                    }
                },
                GET_NUM_STATIONS : {
                    value : "getNumStations",
                    description : "get the number of favorite stations",
                    options : {}
                },
                SUBSCRIBE : {
                    value : "subscribe",
                    description : "subscribe to isOn attribute",
                    options : {}
                },
                UNSUBSCRIBE : {
                    value : "unsubscribe",
                    description : "unsubscribe from isOn attribute",
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
                    case MODES.SET_IS_ON.value:
                        var value;
                        if (!input[1]) {
                            log("please define an option");
                        } else if (input[1] === MODES.SET_IS_ON.options.TRUE) {
                            value = true;
                        } else if (input[1] === MODES.SET_IS_ON.options.FALSE) {
                            value = false;
                        } else {
                            log('invalid option: ' + input[1]);
                        }
                        if (value !== undefined) {
                            radioProxy.isOn.set({
                                value : value
                            }).then(function() {
                                log("radioProxy.isOn.set(" + value + ").then");
                            }).catch(function(error) {
                                log("radioProxy.isOn.set(" + value + ").catch: " + error);
                            });
                        }
                        break;
                    case MODES.GET_IS_ON.value:
                        radioProxy.isOn.get().then(function(value) {
                            prettyLog("radioProxy.isOn.get.then: " + value);
                        }).catch(function(error) {
                            prettyLog("radioProxy.isOn.get.catch: " + error);
                        });
                        break;
                    case MODES.ADD_FAVORITE_STATION.value:
                        if (!input[1]) {
                            log("please define a name");
                        } else {
                            var operationArguments = {
                                radioStation : input[1]
                            };
                            radioProxy.addFavoriteStation(operationArguments).then(
                                    function(returnValue) {
                                        log("radioProxy.addFavoriteStation("
                                            + JSON.stringify(operationArguments)
                                            + ").then. Return value of operation from provider: "
                                            + JSON.stringify(returnValue));
                                    }).catch(
                                    function(error) {
                                        log("radioProxy.addFavoriteStation("
                                            + JSON.stringify(operationArguments)
                                            + ").catch: "
                                            + error);
                                    });
                        }
                        break;
                    case MODES.GET_NUM_STATIONS.value:
                        radioProxy.numberOfStations.get().then(function(value) {
                            prettyLog("radioProxy.numberOfStations.get.then: " + value);
                        }).catch(function(error) {
                            prettyLog("radioProxy.numberOfStations.get.catch: " + error);
                        });
                        break;
                    case MODES.SUBSCRIBE.value:
                        isOnSubscription.subscribe(radioProxy);
                        break;
                    case MODES.UNSUBSCRIBE.value:
                        isOnSubscription.unsubscribe(radioProxy);
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
joynr.load(provisioning, function(error, loadedJoynr) {
    if (error) {
        throw error;
    }
    log("joynr started");
    joynr = loadedJoynr;
    var messagingQos = new joynr.messaging.MessagingQos({
        ttl : 60000
    });
    var RadioProxy = require("../generated/js/joynr/vehicle/RadioProxy.js");
    joynr.proxyBuilder.build(RadioProxy, {
        domain : domain,
        messagingQos : messagingQos
    }).then(function(radioProxy) {
        log("radio proxy build");
        runDemo(radioProxy, function() {
            isOnSubscription.setJoynr(joynr); // TODO this should not be necessary => setting the
                                                // values for OnChangeSubscriptionQos should be
                                                // possible without joynr
            isOnSubscription.subscribe(radioProxy, function() {
                runInteractiveConsole(radioProxy, function() {
                    log("stopping all isOn subscriptions");
                    isOnSubscription.unsubscribe(radioProxy, function() {
                        process.exit(0);
                    });
                });
            });
        });
    }).catch(function(error) {
        log("error building radioProxy: " + error);
    });
});
