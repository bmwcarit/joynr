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

var isOn = false;
var currentStationIndex = 0;
var favouriteStations = ["station1", "station2", "station3"];
var numberOfStations = 0;
var myRadioProvider;

var numberOfStationChanged = function() {
    if(typeof myRadioProvider === "object") {
        myRadioProvider.numberOfStations.valueChanged(parseInt(numberOfStations, 10) + favouriteStations.length);
    } else {
        error("radioProvider.numberOfStationChanged() called but instance of radioProvider not set. Call setProvider(...) first.");
    }
};

exports.setProvider = function(radioProvider) {
    myRadioProvider = radioProvider;
};

exports.implementation = {
    isOn : {
        get : function() {
            prettyLog("radioProvider.isOn.get() called");
            return isOn;
        },
        set : function(value) {
            prettyLog("radioProvider.isOn.set(" + value + ") called");
            isOn = value;
        }
    },
    currentStation : {
        get : function() {
            prettyLog("radioProvider.currentStation.get() called");
            return favouriteStations[currentStationIndex];
        }
    },
    numberOfStations : {
        get : function() {
            prettyLog("radioProvider.numberOfStations.get() called");
            return parseInt(numberOfStations, 10) + favouriteStations.length;
        },
        set : function(value) {
            prettyLog("radioProvider.numberOfStations.set(" + value + ") called");
            numberOfStations = value;
        }
    },
    addFavouriteStation : function(opArgs) {
        prettyLog("radioProvider.addFavouriteStation(" + JSON.stringify(opArgs)
                + ") called");

        if (opArgs === undefined) {
            prettyLog("operation arguments is undefined!");
        }
        if (opArgs.radioStation === undefined) {
            prettyLog("operation argument \"radioStation\" is undefined!");
        }
        favouriteStations.push(opArgs.radioStation);
        numberOfStationChanged();
        return false;
    },
    addFavouriteStationList : function(opArgs) {
        prettyLog("radioProvider.addFavouriteStationList(" + JSON.stringify(opArgs)
                + ") called");
        return false;
    },
    shuffleStations : function(opArgs) {
        prettyLog("radioProvider.shuffleStations(" + JSON.stringify(opArgs)
                + ") called");
        currentStationIndex++;
        currentStationIndex %= favouriteStations.length;
        return false;
    }
};