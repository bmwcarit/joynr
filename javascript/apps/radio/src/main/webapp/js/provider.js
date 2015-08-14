/*jslint devel: true es5: true */
/*global $: true, joynr: true, provisioning: true, RadioProvider: true, RadioStation: true, GeoPosition: true, domain: true, getBuildSignatureString: true, TrafficServiceBroadcastFilter : true, GeocastBroadcastFilter: true */

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

function logHtml(line, msg) {
    $("div#divLog").prepend(
            msg + " <span style=\"font-size:0.8em;color:grey\">" + line + "</span><br>").show();
}
function log(line, msg) {
    logHtml(line, msg);
    msg = new Date().toISOString() + ": [provider.js: " + line + "] " + msg;
    console.log(msg);
}

function showCurrentStationInHtml(radioStation) {
    $("input#txtCurrentStationName").val(radioStation.name);
    $("input#txtCurrentStationTrafficService").val(radioStation.trafficService);
    $("input#txtCurrentStationCountry").val(radioStation.country);
}

function showFavoriteStationsInHtml(radioStations) {
    var i;
    var stationsString = "";
    for (i = 0; i < radioStations.length; i++) {
        if (i !== 0) {
            stationsString += "\n";
        }
        stationsString += "Name: " + radioStations[i].name;
        stationsString += " | TrafficService: " + radioStations[i].trafficService;
        stationsString += " | Country: " + radioStations[i].country;
    }
    $("textarea#txtFavoriteStations").val(stationsString);
}

var joynr;
var currentStationValue = "default value for current station";
var providerQos = null;

var RadioProviderImpl =
        function RadioProviderImpl() {
            var self = this;

            var stationsList = [
                new RadioStation({
                    name : "ABC Trible J",
                    trafficService : true,
                    country : "AUSTRALIA"
                }),
                new RadioStation({
                    name : "Radio Popolare",
                    trafficService : false,
                    country : "ITALY"
                }),
                new RadioStation({
                    name : "JAZZ.FM91",
                    trafficService : false,
                    country : "CANADA"
                }),
                new RadioStation({
                    name : "Bayern 3",
                    trafficService : true,
                    country : "GERMANY"
                })
            ];

            var currentStationIndex = 0;

            Object.defineProperty(this, "currentStationIndex", {
                get : function() {
                    return currentStationIndex;
                }
            });

            Object.defineProperty(this, "stationsList", {
                get : function() {
                    return stationsList;
                }
            });

            this.currentStation = {
                get : function() {
                    log("radioProvider.currentStation.get", "called");
                    return stationsList[currentStationIndex];
                }
            };

            this.addFavoriteStation =
                    function(opArgs) {
                        log("radioProvider.addFavoriteStation", "called with args: "
                            + JSON.stringify(opArgs));

                        if (opArgs === undefined) {
                            log(
                                    "radioProvider.addFavoriteStation",
                                    "operation arguments is undefined!");
                            return;
                        }
                        if (opArgs.newFavoriteStation === undefined) {
                            log(
                                    "radioProvider.addFavoriteStation",
                                    "operation argument \"newFavoriteStation\" is undefined!");
                            return;
                        }

                        stationsList.push(opArgs.newFavoriteStation);

                        showFavoriteStationsInHtml(stationsList);
                        return true;
                    };

            this.shuffleStations =
                    function() {
                        log("radioProvider.shuffleStations", "called");

                        var oldStationIndex = currentStationIndex;

                        currentStationIndex++;
                        currentStationIndex = currentStationIndex % stationsList.length;
                        self.currentStation.valueChanged(stationsList[currentStationIndex]);
                        log("radioProvider.shuffleStations", JSON
                                .stringify(stationsList[oldStationIndex])
                            + " -> "
                            + JSON.stringify(stationsList[currentStationIndex]));

                        showCurrentStationInHtml(stationsList[currentStationIndex]);
                    };

            // Provide broadcast specific implementation stubs. Properties will
            // be added when joynr.providerBuilder.build() gets called.

            this.weakSignal = {};

            this.newStationDiscovered = {};

            // the button onclick handler
            this.fireWeakSignal = function() {
                log("radioProvider.fireWeakSignal", "called");

                var outputParameters;
                outputParameters = self.weakSignal.createBroadcastOutputParameters();
                outputParameters.setWeakSignalStation(stationsList[currentStationIndex]);
                self.weakSignal.fire(outputParameters);
            };

            // the button onclick handler
            this.fireNewStationDiscovered = function() {
                log("radioProvider.fireNewStationDiscovered", "called");
                var outputParameters;
                var geoPosition;

                // can later replace the station with current station
                // and take its position
                geoPosition = new GeoPosition();
                geoPosition.latitude = 48.55;
                geoPosition.longitude = 12.0;
                outputParameters = self.newStationDiscovered.createBroadcastOutputParameters();
                outputParameters.setGeoPosition(geoPosition);
                outputParameters.setDiscoveredStation(stationsList[currentStationIndex]);
                self.newStationDiscovered.fire(outputParameters);
            };

            this.getLocationOfCurrentStation =
                function() {
                    log("radioProvider.getLocationOfCurrentStation", "called");
                    return stationsList[currentStationIndex].country;
                };

            // fill current station into fields
            showCurrentStationInHtml(stationsList[currentStationIndex]);

            // fill favorite stations into textarea
            showFavoriteStationsInHtml(stationsList);
        };

$(function() { // DOM ready
    var radioProvider = null;
    var radioProviderImpl = new RadioProviderImpl();
    // output build signatures to log
    joynr.load(provisioning, function(error, loadedJoynr) {
        if (error) {
            log("main", "error initializing joynr: " + error);
            throw error;
        }

        joynr = loadedJoynr;
        log("main", joynr.buildSignature());
        log("main", getBuildSignatureString());
        // create joynr provider
        radioProvider = joynr.providerBuilder.build(RadioProvider, radioProviderImpl);

        // fill domain into field
        $("input#txtDomain").val(domain);

        // register joynr provider
        $("input#btnRegisterProvider").click(
                function() {
                    domain = $("input#txtDomain").val();

                    // test, whether some filter functions can be added here
                    radioProvider.newStationDiscovered.addBroadcastFilter(new GeocastBroadcastFilter());
                    radioProvider.newStationDiscovered.addBroadcastFilter(new TrafficServiceBroadcastFilter());

                    joynr.capabilities.registerCapability(
                            "RadioProvider.authToken",
                            domain,
                            radioProvider,
                            new joynr.types.ProviderQos({
                                customParameters : [],
                                providerVersion : 1,
                                priority : Date.now(),
                                scope : joynr.types.ProviderScope.GLOBAL,
                                onChangeSubscriptions : true
                            })).then(
                            function() {
                                log(
                                        "joynr.capabilities.registerCapability.done",
                                        "successfully registered Radio provider on domain \""
                                            + domain
                                            + "\".");
                            }).catch(
                            function(error) {
                                log(
                                        "joynr.capabilities.registerCapability.fail",
                                        "error registering Radio provider. Reason: "
                                            + error.toString());
                            });
                    $("input#btnRegisterProvider").attr("disabled", true);
                    $("input#btnUnregisterProvider").attr("disabled", false);
                    $("input#btnShuffleStations").attr("disabled", false);
                    $("input#btnAddFavouriteStation").attr("disabled", false);
                    $("input#btnFireWeakSignal").attr("disabled", false);
                    $("input#btnFireNewStationDiscovered").attr("disabled", false);
                });

        // unregister joynr provider
        $("input#btnUnregisterProvider").click(
                function() {
                    joynr.capabilities.unregisterCapability(
                            "RadioProvider.authToken",
                            domain,
                            radioProvider).then(
                            function() {
                                log(
                                        "joynr.capabilities.unregisterCapability.done",
                                        "successfully unregistered Radio provider on domain \""
                                            + domain
                                            + "\".");
                            }).catch(
                            function(error) {
                                log(
                                        "joynr.capabilities.unregisterCapability.fail",
                                        "error unregistering Radio provider. Reason: " + error);
                            });
                    $("input#btnRegisterProvider").attr("disabled", false);
                    $("input#btnUnregisterProvider").attr("disabled", true);
                    $("input#btnShuffleStations").attr("disabled", true);
                    $("input#btnAddFavouriteStation").attr("disabled", true);
                    $("input#btnFireWeakSignal").attr("disabled", true);
                    $("input#btnFireNewStationDiscovered").attr("disabled", true);
                });

        $("input#btnShuffleStations").click(function() {
            radioProviderImpl.shuffleStations();
        });
        $("input#btnFireWeakSignal").click(function() {
            radioProviderImpl.fireWeakSignal();
        });
        $("input#btnFireNewStationDiscovered").click(function() {
            radioProviderImpl.fireNewStationDiscovered();
        });
        $("input#btnAddFavoriteStation").click(function() {
            radioProviderImpl.addFavoriteStation({
                newFavoriteStation : new RadioStation({
                    name : $("input#txtNewFavoriteStationName").val(),
                    trafficService : $("select#slctTrafficService").val() === "true",
                    country : $("select#slctCountry").val()
                })
            });
        });
    });
});
