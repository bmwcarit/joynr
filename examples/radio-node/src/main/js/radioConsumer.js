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
var joynr = require("joynr");
var RadioStation;
var Country;
var currentStationSubscriptionId;
var subscriptionQosOnChange;

var runDemo = function(radioProxy) {
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
                SHUFFLE_STATIONS : {
                    value : "s",
                    description : "shuffle stations",
                    options : {}
                },
                ADD_FAVORITE_STATION : {
                    value : "a",
                    description : "add a Favorite Station",
                    options : {
                        NAME : "name"
                    }
                },
                SUBSCRIBE : {
                    value : "subscribe",
                    description : "subscribe to current station",
                    options : {}
                },
                UNSUBSCRIBE : {
                    value : "unsubscribe",
                    description : "unsubscribe from current station",
                    options : {}
                },
                GET_CURRENT_STATION : {
                    value : "c",
                    description : "get current station",
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
                    case MODES.ADD_FAVORITE_STATION.value:
                        var newFavoriteStation = new RadioStation({
                            name : input[1] || "",
                            trafficService : true,
                            country : Country.GERMANY
                        });

                        radioProxy.addFavoriteStation({newFavoriteStation: newFavoriteStation})
                            .then(
                                function(returnValue) {
                                    prettyLog("RPC: radioProxy.addFavoriteStation("
                                        + JSON.stringify(newFavoriteStation)
                                        + ") returned: "
                                        + JSON.stringify(returnValue));
                                    return returnValue;
                                }).catch(
                                function(error) {
                                    prettyLog("RPC: radioProxy.addFavoriteStation("
                                        + JSON.stringify(newFavoriteStation)
                                        + ") failed."
                                        + error);
                                    return error;
                                });
                        break;
                    case MODES.SHUFFLE_STATIONS.value:
                        radioProxy.shuffleStations().then(function() {
                            prettyLog("RPC: radioProxy.shuffleStations returned. ");
                            return null;
                        }).catch(function(error) {
                            prettyLog("RPC: radioProxy.shuffleStations failed: " + error);
                            return error;
                       });
                        break;
                    case MODES.SUBSCRIBE.value:
                        if (currentStationSubscriptionId === undefined) {
                            var currentStationOnReceiveCallback = function currentStationOnReceiveCallback(value) {
                                prettyLog("radioProxy.currentStation.subscribe.onReceive",
                                          JSON.stringify(value));
                            };
                            var currentStationOnErrorCallback = function currentStationOnErrorCallback() {
                                prettyLog("radioProxy.currentStation.subscribe.onPublicationMissed",
                                          "publication missed");
                            };

                            radioProxy.currentStation.subscribe({
                                subscriptionQos : subscriptionQosOnChange,
                                onReceive : currentStationOnReceiveCallback,
                                onError : currentStationOnErrorCallback
                            }).then(function(subscriptionId) {
                                currentStationSubscriptionId = subscriptionId;
                                prettyLog("radioProxy.currentStation.subscribe.done",
                                          "Subscription ID: "+ subscriptionId);
                                return subscriptionId;
                            }).catch(function(error) {
                                prettyLog("radioProxy.currentStation.subscribe.fail", error);
                                return error;
                            });
                        }
                        break;
                    case MODES.UNSUBSCRIBE.value:
                        if (currentStationSubscriptionId !== undefined) {
                            radioProxy.currentStation.unsubscribe({
                                "subscriptionId" : currentStationSubscriptionId
                            }).then(function() {
                                prettyLog("radioProxy.currentStation.unsubscribe.done",
                                          "Subscription ID: " + currentStationSubscriptionId);
                                currentStationSubscriptionId = undefined;
                            }).catch(function(error) {
                                prettyLog("radioProxy.currentStation.unsubscribe.fail",
                                          "Subscription ID: " + currentStationSubscriptionId + " ERROR" + error
                                );
                            });
                        }
                        break;
                    case MODES.GET_CURRENT_STATION.value:
                        radioProxy.currentStation.get().then(function(currentStation) {
                            prettyLog("RPC: radioProxy.getCurrentStation returned: " + JSON.stringify(currentStation));
                        }).catch(function(error) {
                            prettyLog("RPC: radioProxy.getCurrentStation failed: " + error);
                        });
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

var provisioning = require("./provisioning_common.js");
RadioStation = require("../generated/js/joynr/vehicle/RadioStation");
Country = require("../generated/js/joynr/vehicle/Country");
require("../generated/js/joynr/vehicle/GeoPosition");
var AddFavoriteStationErrorEnum = require("../generated/js/joynr/vehicle/Radio/AddFavoriteStationErrorEnum");
var RadioProxy = require("../generated/js/joynr/vehicle/RadioProxy.js");
joynr.load(provisioning).then(function(loadedJoynr) {
    log("joynr started");
    joynr = loadedJoynr;
    var messagingQos = new joynr.messaging.MessagingQos({
        ttl : 60000
    });

    subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
        minInterval : 50
	});

    joynr.proxyBuilder.build(RadioProxy, {
        domain : domain,
        messagingQos : messagingQos
    }).then(function(radioProxy) {
        log("radio proxy build");
        runDemo(radioProxy);
        runInteractiveConsole(radioProxy, function() {
            log("exiting...");
            process.exit(0);
        });
        return radioProxy;
    }).catch(function(error) {
        log("error running radioProxy: " + error);
    });
    return loadedJoynr;
}).catch(function(error){
    throw error;
});
