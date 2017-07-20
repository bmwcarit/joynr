/*jslint node: true */

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

var prettyLog = require("./logging.js").prettyLog;
var error = require("./logging.js").error;

var joynr = require("joynr");
var RadioStation = require("../generated/js/joynr/vehicle/RadioStation");
var Country = require("../generated/js/joynr/vehicle/Country");
var GeoPosition = require("../generated/js/joynr/vehicle/GeoPosition");
var AddFavoriteStationErrorEnum = require("../generated/js/joynr/vehicle/Radio/AddFavoriteStationErrorEnum");

var MyRadioProvider = function() {
    var self = this;
    var currentStationIndex = 0;
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
    var myRadioProvider;
    var stationsList = [
                        new RadioStation({
                            name : "ABC Trible J",
                            trafficService : false,
                            country : Country.AUSTRALIA
                        }),
                        new RadioStation({
                            name : "Radio Popolare",
                            trafficService : true,
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
    this.setProvider = function(radioProvider) {
        myRadioProvider = radioProvider;
    };
    this.currentStation = {
        get : function() {
            prettyLog("radioProvider.currentStation.get() called");
            return stationsList[currentStationIndex];
        }
    };
    this.addFavoriteStation = function(opArgs) {
        prettyLog("radioProvider.addFavoriteStation(" + JSON.stringify(opArgs)
                + ") called");

        if (opArgs === undefined) {
            prettyLog("operation arguments is undefined!");
            return {
                success : false
            };
        }
        if (opArgs.newFavoriteStation === undefined) {
            prettyLog("operation argument \"newFavoriteStation\" is undefined!");
            return {
                success : false
            };
        }
        if (opArgs.newFavoriteStation.name === "") {
            throw new joynr.exceptions.ProviderRuntimeException({
               detailMessage: "MISSING_NAME"
            });
        }

        var i;
        // copy over and type each single member
        for (i in stationsList) {
            if (stationsList.hasOwnProperty(i)) {
                if (stationsList[i].name === opArgs.newFavoriteStation.name) {
                    throw AddFavoriteStationErrorEnum.DUPLICATE_RADIOSTATION;
                }
            }
        }

        stationsList.push(opArgs.newFavoriteStation);
        return {
            success : true
        };
    };

    this.addFavoriteStationList = function(opArgs) {
        prettyLog("radioProvider.addFavoriteStationList(" + JSON.stringify(opArgs)
                + ") called");
        return false;
    };

    this.shuffleStations = function(opArgs) {
        prettyLog("radioProvider.shuffleStations(" + JSON.stringify(opArgs)
                + ") called");
        currentStationIndex++;
        currentStationIndex %= stationsList.length;

        self.currentStation.valueChanged(stationsList[currentStationIndex]);
        return false;
    };

    this.fireWeakSignal = function() {
        var broadcast = myRadioProvider.weakSignal;
        var outputParams = broadcast.createBroadcastOutputParameters();
        outputParams.setWeakSignalStation(stationsList[currentStationIndex]);
        broadcast.fire(outputParams);
    };

    this.fireWeakSignalWithPartition = function() {
        var broadcast = myRadioProvider.weakSignal;
        var outputParams = broadcast.createBroadcastOutputParameters();
        var currentStation = stationsList[currentStationIndex];
        outputParams.setWeakSignalStation(currentStation);
        broadcast.fire(outputParams, [ currentStation.country.name ]);
    };

    this.getLocationOfCurrentStation = function() {
        prettyLog("radioProvider.getLocationOfCurrentStation",
                  "called");
        return {
            country : stationsList[currentStationIndex].country,
            location : countryGeoPositionMap[stationsList[currentStationIndex].country.name]
        };
    };
};

module.exports = MyRadioProvider;
