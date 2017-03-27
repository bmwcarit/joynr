/*jslint devel: true es5: true */
/*global $: true, joynr: true, provisioning: true, Country: true, AddFavoriteStationErrorEnum: true, RadioProvider: true, RadioStation: true, GeoPosition: true, domain: true, getBuildSignatureString: true, TrafficServiceBroadcastFilter : true, GeocastBroadcastFilter: true, Promise: true */

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
    $("input#txtCurrentStationCountry").val(radioStation.country.name);
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
        stationsString += " | Country: " + radioStations[i].country.name;
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
                    country : Country.AUSTRALIA
                }),
                new RadioStation({
                    name : "Radio Popolare",
                    trafficService : false,
                    country : Country.ITALY
                }),
                new RadioStation({
                    name : "JAZZ.FM91",
                    trafficService : false,
                    country : Country.CANADA
                }),
                new RadioStation({
                    name : "Bayern 3",
                    trafficService : true,
                    country : Country.GERMANY
                })
            ];

            var countryGeoPositionMap = {
                // Melbourne
                "AUSTRALIA": new GeoPosition({ latitude: -37.8141070, longitude: 144.9632800 }),
                // Bolzano
                "ITALY": new GeoPosition({ latitude: 46.4982950, longitude: 11.3547580 }),
                // Edmonton
                "CANADA": new GeoPosition({ latitude: 53.5443890, longitude: -113.4909270 }),
                // Munich
                "GERMANY": new GeoPosition({ latitude: 48.1351250, longitude: 11.5819810 })
            };

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
                        return new Promise(function(resolve, reject) {
                            /*
                             * synchronous method implementation can be used if no
                             * further asynchronous activity is required to obtain
                             * the result.
                             * In case of error a ProviderRuntimeException or an
                             * error enum value suitable for the method can be
                             * thrown. The latter will be automatically wrapped
                             * inside an ApplicationException.
                             */
                            log("radioProvider.addFavoriteStation", "called with args: "
                                + JSON.stringify(opArgs));

                            if (opArgs === undefined) {
                                log(
                                        "radioProvider.addFavoriteStation",
                                        "operation arguments is undefined!");
                                reject(new joynr.exceptions.ProviderRuntimeException({ detailMessage: "operation arguments is undefined!" }));
                            }
                            else if (opArgs.newFavoriteStation === undefined) {
                                log(
                                        "radioProvider.addFavoriteStation",
                                        "operation argument \"newFavoriteStation\" is undefined!");
                                reject(new joynr.exceptions.ProviderRuntimeException({ detailMessage: "operation argument \"newFavoriteStation\" is undefined!" }));
                            }
                            else if (opArgs.newFavoriteStation.name === "") {
                                reject(new joynr.exceptions.ProviderRuntimeException({ detailMessage: "MISSING_NAME" }));
                            }
                            else {
                                var duplicateFound = false, i;
                                // copy over and type each single member
                                for (i in stationsList) {
                                    if (stationsList.hasOwnProperty(i)) {
                                        if (!duplicateFound && stationsList[i].name === opArgs.newFavoriteStation.name) {
                                            duplicateFound = true;
                                            reject(AddFavoriteStationErrorEnum.DUPLICATE_RADIOSTATION);
                                            break;
                                        }
                                    }
                                }
                                if (!duplicateFound) {
                                    stationsList.push(opArgs.newFavoriteStation);

                                    showFavoriteStationsInHtml(stationsList);
                                    resolve({
                                        success : true
                                    });
                                }
                            }
                        });

                    };

            this.shuffleStations =
                    function() {
                        /*
                         * asynchronous method implementation is required if
                         * the result depends on other asynchronous activity.
                         *
                         * When activity is finished, then based on the positive
                         * or negative outcome, resolve() or reject() has to be
                         * called. For resolve() any output parameters have to be
                         * provided. For reject a ProviderRuntimeException or an
                         * error enum value suitable for the method must be
                         * provided. The latter will be automatically wrapped
                         * inside an ApplicationException.
                         */
                        log("radioProvider.shuffleStations", "called");
                        return new Promise(function(resolve, reject) {
                            var oldStationIndex = currentStationIndex;
                            currentStationIndex++;
                            currentStationIndex = currentStationIndex % stationsList.length;
                            self.currentStation.valueChanged(stationsList[currentStationIndex]);
                            log("radioProvider.shuffleStations", JSON
                                    .stringify(stationsList[oldStationIndex])
                                + " -> "
                                + JSON.stringify(stationsList[currentStationIndex]));

                            showCurrentStationInHtml(stationsList[currentStationIndex]);
                            resolve();
                        });
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

                geoPosition = countryGeoPositionMap[stationsList[currentStationIndex].country.name];
                outputParameters = self.newStationDiscovered.createBroadcastOutputParameters();
                outputParameters.setGeoPosition(geoPosition);
                outputParameters.setDiscoveredStation(stationsList[currentStationIndex]);
                self.newStationDiscovered.fire(outputParameters);
            };

            this.getLocationOfCurrentStation =
                function() {
                    log("radioProvider.getLocationOfCurrentStation", "called");
                    return {
                        country : stationsList[currentStationIndex].country,
                        location : countryGeoPositionMap[stationsList[currentStationIndex].country.name]
                    };
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
    joynr.load(provisioning).then(function(loadedJoynr){
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

                    joynr.registration.registerProvider(
                            domain,
                            radioProvider,
                            new joynr.types.ProviderQos({
                                customParameters : [],
                                priority : Date.now(),
                                scope : joynr.types.ProviderScope.GLOBAL,
                                supportsOnChangeSubscriptions : true
                            })).then(
                            function() {
                                log(
                                        "joynr.registration.registerProvider.done",
                                        "successfully registered Radio provider on domain \""
                                            + domain
                                            + "\".");
                            }).catch(
                            function(error) {
                                log(
                                        "joynr.registration.registerProvider.fail",
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
                    joynr.registration.unregisterProvider(
                            domain,
                            radioProvider).then(
                            function() {
                                log(
                                        "joynr.registration.unregisterProvider.done",
                                        "successfully unregistered Radio provider on domain \""
                                            + domain
                                            + "\".");
                            }).catch(
                            function(error) {
                                log(
                                        "joynr.registration.unregisterProvider.fail",
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
    }).catch(function(error) {
        log("main", "error initializing joynr: " + error);
        throw error;
    });
});
