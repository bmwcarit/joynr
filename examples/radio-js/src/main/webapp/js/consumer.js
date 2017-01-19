/*jslint devel: true es5: true */
/*global $: true, joynr: true, provisioning: true, RadioProxy: true, GeoPosition: true, BroadcastFilterParameters: true, RadioStation: true, domain: true, getBuildSignatureString: true */

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

var currentStationSubscriptionId;

var messagingQos, subscriptionQosOnChange, subscriptionQosPeriodic, subscriptionQosOnChangeWithKeepAlive;

var filterGeoPositionMap = {
    // Geelong, distance to Melbourne ~75 km by car
    "AUSTRALIA": new GeoPosition({ latitude: -38.149251, longitude: 144.364415 }),
    // Meran, distance to Bolzano ~34 km by car
    "ITALY": new GeoPosition({ latitude: 46.670286, longitude: 11.152680 }),
    // Redwater, distance to Edmonton ~62 km by car
    "CANADA": new GeoPosition({ latitude: 53.950466, longitude: -113.107809 }),
    // Nuremberg, distance to Munich ~170 km by car
    "GERMANY": new GeoPosition({ latitude: 49.444336, longitude: 11.071728 })
};

function logHtml(line, msg) {
    $("div#divLog").prepend(msg+" <span style=\"font-size:0.8em;color:grey\">"+line+"</span><br>").show();
}

function log(line, msg) {
     logHtml(line, msg);
     msg = new Date().toISOString() + ": [provider.js: " + line + "] " + msg;
     console.log(msg);
}

function showCurrentStationInHtml(radioStation) {
    $("input#txtCurrentStationName").val(radioStation.name);
    $("input#txtCurrentStationTrafficService").val(radioStation.trafficService);
    $("input#txtCurrentStationCountry").val(radioStation.country.name);
}

function enableSubscriptionButtons(enable) {
    $("input#btnCurrentStationSubscribeOnChange").attr("disabled", !enable);
    $("input#btnCurrentStationSubscribePeriodic").attr("disabled", !enable);
    $("input#btnCurrentStationSubscribeOnChangeWithKeepAlive").attr("disabled", !enable);
    $("input#btnCurrentStationUnsubscribe").attr("disabled", enable);
}

/**
 * @param {RadioProxy}
 *            radioProxy
 */
function registerAttributeHandlers(radioProxy) {
    $("input#btnCurrentStationGet").click(function() {
        radioProxy.currentStation.get().then(function(value) {
            log("radioProxy.currentStation.get.done", JSON.stringify(value));
            showCurrentStationInHtml(value);
        }).catch(function(error) {
            log("radioProxy.currentStation.get.fail", error);
        });
    });

    function onCurrentStationReceive(value) {
        log("radioProxy.currentStation.subscribe.onReceive", JSON.stringify(value));
        showCurrentStationInHtml(value);
    }

    function onCurrentStationPublicationMissed() {
        log("radioProxy.currentStation.subscribe.onPublicationMissed", "publication missed");
    }

    $("input#btnCurrentStationSubscribeOnChange").click(function() {
        radioProxy.currentStation.subscribe(
                {
                    subscriptionQos : subscriptionQosOnChange,
                    onReceive : onCurrentStationReceive,
                    onError : onCurrentStationPublicationMissed
                }
        ).then(function(subscriptionId) {
            currentStationSubscriptionId = subscriptionId;
            enableSubscriptionButtons(false);
            log("radioProxy.currentStation.subscribe.done", "Subscription ID: "+ subscriptionId);
        }).catch(function(error) {
            log("radioProxy.currentStation.subscribe.fail", error);
        });
    });

    $("input#btnCurrentStationSubscribePeriodic").click(function() {
        radioProxy.currentStation.subscribe(
                {
                    subscriptionQos : subscriptionQosPeriodic,
                    onReceive : onCurrentStationReceive,
                    onError : onCurrentStationPublicationMissed
                }
        ).then(function(subscriptionId) {
            currentStationSubscriptionId = subscriptionId;
            enableSubscriptionButtons(false);
            log("radioProxy.currentStation.subscribe.done", "Subscription ID: "+ subscriptionId);
        }).catch(function(error) {
            log("radioProxy.currentStation.subscribe.fail", error);
        });
    });

    $("input#btnCurrentStationSubscribeOnChangeWithKeepAlive").click(function() {
        radioProxy.currentStation.subscribe(
                {
                    subscriptionQos : subscriptionQosOnChangeWithKeepAlive,
                    onReceive : onCurrentStationReceive,
                    onError : onCurrentStationPublicationMissed
                }
        ).then(function(subscriptionId) {
            currentStationSubscriptionId = subscriptionId;
            enableSubscriptionButtons(false);
            log("radioProxy.currentStation.subscribe.done", "Subscription ID: "+ subscriptionId);
        }).catch(function(error) {
            log("radioProxy.currentStation.subscribe.fail", error);
        });
    });

    $("input#btnCurrentStationUnsubscribe").click(function() {
        log("radioProxy.currentStation.unsubscribe", "Subscription ID: " + currentStationSubscriptionId);
        radioProxy.currentStation.unsubscribe({
            "subscriptionId" : currentStationSubscriptionId
        }).then(function() {
            log("radioProxy.currentStation.unsubscribe.done", "Subscription ID: " + currentStationSubscriptionId);
            currentStationSubscriptionId = null;
            enableSubscriptionButtons(true);
        }).catch(function(error) {
            log(
                    "radioProxy.currentStation.unsubscribe.fail",
                    "Subscription ID: " + currentStationSubscriptionId + " ERROR" + error
            );
        });
    });
}

/**
 * @param {RadioProxy}
 *            radioProxy
 */
function registerMethodHandlers(radioProxy) {
    $("input#btnShuffleStations").click(function() {
        log("radioProxy.shuffleStations", "calling shuffleStations");
        radioProxy.shuffleStations().then(function() {
            log("radioProxy.shuffleStations.done", "successfully shuffled stations");
        }).catch(function(error) {
            log("radioProxy.shuffleStations.fail", error);
        });
    });

    $("input#btnAddFavoriteStation").click(function() {
        var operationArguments = {
            newFavoriteStation : new RadioStation({
                name : $("input#txtNewFavoriteStationName").val(),
                trafficService : $("select#slctTrafficService").val() === "true",
                country : $("select#slctCountry").val()
            })
        };

        radioProxy.addFavoriteStation(operationArguments).then(function(opArgs) {
            var success = opArgs.success;
            log(
                    "radioProxy.addFavoriteStation.done",
                    JSON.stringify(operationArguments) + " -> " + JSON.stringify(success)
            );
            $("input#txtAddFavoriteStationSuccess").val(JSON.stringify(success));
        }).catch(function(error) {
            log(
                    "radioProxy.addFavoriteStation.failed",
                    JSON.stringify(operationArguments) + " error: " + JSON.stringify(error)
            );
        });
    });

    $("input#btnGetLocationOfCurrentStation").click(function() {
        radioProxy.getLocationOfCurrentStation().then(function(opArgs) {
            var country = opArgs.country;
            var location = opArgs.location;
            log(
                    "radioProxy.getLocationOfCurrentStation.done",
                    "Country: " + JSON.stringify(country) + ", location: " + JSON.stringify(location)
            );
            $("input#txtGetLocationOfCurrentStation").val("Country: " + JSON.stringify(country) + ", location: " + JSON.stringify(location));
        }).catch(function(error) {
            log(
                    "radioProxy.getLocationOfCurrentStation.failed",
                    "Error: " + JSON.stringify(error)
            );
        });
    });
}

/**
 * @param {RadioProxy}
 *            radioProxy
 */
function registerEventHandlers(radioProxy) {
    var subscriptionToWeakSignalId = null;
    var subscriptionToNewStationDiscoveredId = null;

    // intialize
    $("#lat").val(filterGeoPositionMap.AUSTRALIA.latitude.toString());
    $("#lon").val(filterGeoPositionMap.AUSTRALIA.longitude.toString());

    // change according to selection of country
    $("#filterCountry").change(function() {
        var value = $(this).val();
        console.log("New value is: " + value);
        $("#lat").val(filterGeoPositionMap[value].latitude.toString());
        $("#lon").val(filterGeoPositionMap[value].longitude.toString());
    });

    // weak signal broadcast

    $("#btnSubscribeToWeakSignal").click(function() {
        if (!subscriptionToWeakSignalId) {
            $("input#btnSubscribeToWeakSignal").attr("disabled", true);
            radioProxy.weakSignal.subscribe({
                onReceive : function(weakSignalStation) {
                    var weakSignalStationReadable = JSON.stringify(weakSignalStation);
                    log(
                            "radioProxy.weakSignal.onReceive",
                            "radioProxy.weakSignal.publication: " + weakSignalStationReadable
                    );
                    $("div#divBroadcasts").prepend("broadcast received: " + weakSignalStationReadable + "<br>").show();
                }
            }).then(function(newSubscriptionId) {
                log(
                        "radioProxy.weakSignal.subscribe",
                        "subscribe done: " + newSubscriptionId
                );
                subscriptionToWeakSignalId = newSubscriptionId;
                $("input#btnUnsubscribeFromWeakSignal").attr("disabled", false);
                $("input#btnSubscribeToWeakSignalUpdate").attr("disabled", false);
            }, function(error) {
                log(
                        "radioProxy.weakSignal.subscribe",
                        "subscribe failed: " + error
                );
                $("input#btnSubscribeToWeakSignal").attr("disabled", false);
                $("input#btnSubscribeToWeakSignalUpdate").attr("disabled", true);
                });
        } else {
            log(
                    "btnSubscribeToNewStationDiscovered click",
                    "there is already a pending attribute subscription"
            );
        }
    });

    $("#btnSubscribeToWeakSignalUpdate").click(function() {
        if (subscriptionToWeakSignalId) {
            $("input#btnSubscribeToWeakSignal").attr("disabled", true);
            $("input#btnSubscribeToWeakSignalUpdate").attr("disabled", true);
            radioProxy.weakSignal.subscribe({
                onReceive : function(weakSignalStation) {
                    var weakSignalStationReadable = JSON.stringify(weakSignalStation);
                    log(
                            "radioProxy.weakSignal.onReceive after update",
                            "radioProxy.weakSignal.publication: " + weakSignalStationReadable
                    );
                    $("div#divBroadcasts").prepend("broadcast received after update: " + weakSignalStationReadable + "<br>").show();
                },
                subscriptionId: subscriptionToWeakSignalId
            }).then(function(newSubscriptionId) {
                log(
                        "radioProxy.weakSignal.subscribe update",
                        "subscribe update done: " + newSubscriptionId
                );
                subscriptionToWeakSignalId = newSubscriptionId;
                $("input#btnUnsubscribeFromWeakSignal").attr("disabled", false);
            }, function(error) {
                log(
                        "radioProxy.weakSignal.subscribe update",
                        "subscribe update failed: " + error
                );
                $("input#btnSubscribeToWeakSignal").attr("disabled", false);
                $("input#btnSubscribeToWeakSignalUpdate").attr("disabled", false);
                });
        } else {
            log(
                    "btnSubscribeToNewStationDiscoveredUpdate click",
                    "there is no attribute subscription that could be updated"
            );
        }
    });

    $("#btnUnsubscribeFromWeakSignal").click(function() {
        if (subscriptionToWeakSignalId) {
            $("input#btnUnsubscribeFromWeakSignal").attr("disabled", true);
            radioProxy.weakSignal.unsubscribe({
                "subscriptionId" : subscriptionToWeakSignalId
            }).then(function() {
                log(
                    "radioProxy.weakSignal.unsubscribe",
                    "unsubscribe done"
                );
                $("input#btnSubscribeToWeakSignal").attr("disabled", false);
                $("input#btnSubscribeToWeakSignalUpdate").attr("disabled", true);
            }).catch(function(error) {
                log(
                    "radioProxy.weakSignal.unsubscribe",
                    "unsubscribe failed" + error
                );
                $("input#btnUnsubscribeFromWeakSignal").attr("disabled", false);
                $("input#btnSubscribeToWeakSignalUpdate").attr("disabled", false);
            });

            subscriptionToWeakSignalId = null;
        } else {
            log("there is no pending weak signal subscription to be cancelled");
        }
    });

    // new station discovered broadcast

    $("#btnSubscribeToNewStationDiscoveredSignal").click(function() {
        if (!subscriptionToNewStationDiscoveredId) {
            $("input#btnSubscribeToNewStationDiscoveredSignal").attr("disabled", true);

            // setup filter parameters
            var geoPosition = filterGeoPositionMap[$("select#filterCountry").val()];
            var positionOfInterest = JSON.stringify(geoPosition);
            // distance is needed as string
            var distance = $("select#filterDistance").val();
            // hasTrafficService is needed as string
            var hasTrafficService = $("select#filterTrafficService").val();
            var myFilterParameters = radioProxy.newStationDiscovered.createFilterParameters();
            myFilterParameters.setHasTrafficService(hasTrafficService);
            myFilterParameters.setPositionOfInterest(positionOfInterest);
            myFilterParameters.setRadiusOfInterestArea(distance);

            radioProxy.newStationDiscovered.subscribe({
                onReceive : function(newStationDiscovered) {
                    var newStationDiscoveredReadable = JSON.stringify(newStationDiscovered);
                    log(
                            "radioProxy.newStationDiscovered.onReceive",
                            "radioProxy.newStationDiscovered.publication: " + newStationDiscoveredReadable
                    );
                    $("div#divBroadcasts").prepend("broadcast received: " + newStationDiscoveredReadable + "<br>").show();
                },
                filterParameters: myFilterParameters
            }).then(function(newSubscriptionId) {
                log(
                        "radioProxy.newStationDiscoveredSignal.subscribe",
                        "subscribe done: " + newSubscriptionId
                );
                subscriptionToNewStationDiscoveredId = newSubscriptionId;
                $("input#btnUnsubscribeFromNewStationDiscoveredSignal").attr("disabled", false);
            }, function(error) {
                log(
                        "radioProxy.newStationDiscoveredSignal.subscribe",
                        "subscribe failed: " + error
                );
                $("input#btnSubscribeToNewStationDiscoveredSignal").attr("disabled", false);
            });
        } else {
            log(
                    "btnSubscribeToNewStationDiscovered click",
                    "there is already a pending attribute subscription"
            );
        }
    });

    $("#btnUnsubscribeFromNewStationDiscoveredSignal").click(function() {
        if (subscriptionToNewStationDiscoveredId) {
            $("input#btnUnsubscribeFromNewStationDiscoveredSignal").attr("disabled", true);
            radioProxy.newStationDiscovered.unsubscribe({
                "subscriptionId" : subscriptionToNewStationDiscoveredId
            }).then(function() {
                log(
                    "radioProxy.newStationDiscovered.unsubscribe",
                    "unsubscribe done"
                );
                $("input#btnSubscribeToNewStationDiscoveredSignal").attr("disabled", false);
            }).catch(function(error) {
                log(
                    "radioProxy.newStationDiscovered.unsubscribe",
                    "unsubscribe failed: " + error
                );
                $("input#btnUnsubscribeFromNewStationDiscoveredSignal").attr("disabled", false);
            });

            subscriptionToNewStationDiscoveredId = null;
        } else {
            log("there is no pending newStationDiscovered subscription to be cancelled");
        }
    });
}

$(function() { // DOM ready
    // output build signatures to log
    joynr.load(provisioning).then(function(loadedJoynr){
        log("main", joynr.buildSignature());
        log("main", getBuildSignatureString());

        joynr = loadedJoynr;

        // fill domain into field
        $("input#txtDomain").val(domain);

        messagingQos = new joynr.messaging.MessagingQos({
            ttl : 60000
        });

        subscriptionQosOnChange = new joynr.proxy.OnChangeSubscriptionQos({
            minIntervalMs : 50
        });

        subscriptionQosPeriodic = new joynr.proxy.PeriodicSubscriptionQos({
            periodMs : 1000
        });

        subscriptionQosOnChangeWithKeepAlive = new joynr.proxy.OnChangeWithKeepAliveSubscriptionQos({
            minIntervalMs : 500,
            maxIntervalMs : 2000
        });

        // create proxy
        $("input#btnCreateProxy").click(function() {
            domain = $("input#txtDomain").val();
            joynr.proxyBuilder.build(RadioProxy, {
                domain : domain,
                messagingQos : messagingQos
            }).then(function(radioProxy) {
                log(
                        "joynr.proxyBuilder.build.done",
                        "successfully discovered radio provider on domain \""+domain+"\"."
                );
                registerAttributeHandlers(radioProxy);
                registerMethodHandlers(radioProxy);
                registerEventHandlers(radioProxy);
            }).catch(function(error) {
                log(
                        "joynr.proxyBuilder.build.fail",
                        "error discovering radio provider: " + error
                );
            });
            $("input#btnCreateProxy").attr("disabled", true);
            // enable buttons to use proxy
            $("input#btnCurrentStationSubscribeOnChange").attr("disabled", false);
            $("input#btnCurrentStationSubscribePeriodic").attr("disabled", false);
            $("input#btnCurrentStationSubscribeOnChangeWithKeepAlive").attr("disabled", false);
            $("input#btnCurrentStationGet").attr("disabled", false);
            $("input#btnShuffleStations").attr("disabled", false);
            $("input#btnAddFavoriteStation").attr("disabled", false);
            $("input#btnGetLocationOfCurrentStation").attr("disabled", false);
            $("input#btnSubscribeToWeakSignal").attr("disabled", false);
            $("input#btnSubscribeToNewStationDiscoveredSignal").attr("disabled", false);
         });
    }).catch(function(error){
        log("main", "error initializing joynr: " + error);
        throw error;
    });
});
