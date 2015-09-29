/*jslint node: true */

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

var prettyLog = require("./logging.js").prettyLog;
var error = require("./logging.js").error;

var currentStationIndex = 0;

var RadioStation = require("../generated/js/joynr/vehicle/RadioStation");

var stationsList = [
                new RadioStation({
                    name : "ABC Trible J",
                    trafficService : false,
                    country : "AUSTRALIA"
                }),
                new RadioStation({
                    name : "Radio Popolare",
                    trafficService : true,
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

var myRadioProvider;

exports.setProvider = function(radioProvider) {
    myRadioProvider = radioProvider;
};

var self;

exports.implementation = {
    currentStation : {
        get : function() {
            prettyLog("radioProvider.currentStation.get() called");
            return stationsList[currentStationIndex];
        }
    },
    addFavoriteStation : function(opArgs) {
        prettyLog("radioProvider.addFavoriteStation(" + JSON.stringify(opArgs)
                + ") called");

        if (opArgs === undefined) {
            prettyLog("operation arguments is undefined!");
        }
        if (opArgs.newFavoriteStation === undefined) {
            prettyLog("operation argument \"newFavoriteStation\" is undefined!");
        }
        stationsList.push(opArgs.newFavoriteStation);
        return false;
    },
    addFavoriteStationList : function(opArgs) {
        prettyLog("radioProvider.addFavoriteStationList(" + JSON.stringify(opArgs)
                + ") called");
        return false;
    },
    shuffleStations : function(opArgs) {
        prettyLog("radioProvider.shuffleStations(" + JSON.stringify(opArgs)
                + ") called");
        currentStationIndex++;
        currentStationIndex %= stationsList.length;
        self.currentStation.valueChanged(stationsList[currentStationIndex]);
        return false;
    },
    getLocationOfCurrentStation : function() {
        prettyLog("radioProvider.getLocationOfCurrentStation",
                  "called");
        return stationsList[currentStationIndex].country;
    }

};

self = exports.implementation;