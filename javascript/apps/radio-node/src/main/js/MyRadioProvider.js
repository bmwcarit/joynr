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
var favoriteStations = ["station1", "station2", "station3"];
var numberOfStations = 0;
var myRadioProvider;

var numberOfStationChanged = function() {
    if(typeof myRadioProvider === "object") {
        myRadioProvider.numberOfStations.valueChanged(parseInt(numberOfStations, 10) + favoriteStations.length);
    } else {
        error("radioProvider.numberOfStationChanged() called but instance of radioProvider not set. Call setProvider(...) first.");
    }
};

exports.setProvider = function(radioProvider) {
    myRadioProvider = radioProvider;
};

exports.implementation = {
    currentStation : {
        get : function() {
            prettyLog("radioProvider.currentStation.get() called");
            return favoriteStations[currentStationIndex];
        }
    },
    numberOfStations : {
        get : function() {
            prettyLog("radioProvider.numberOfStations.get() called");
            return parseInt(numberOfStations, 10) + favoriteStations.length;
        },
        set : function(value) {
            prettyLog("radioProvider.numberOfStations.set(" + value + ") called");
            numberOfStations = value;
        }
    },
    addFavoriteStation : function(opArgs) {
        prettyLog("radioProvider.addFavoriteStation(" + JSON.stringify(opArgs)
                + ") called");

        if (opArgs === undefined) {
            prettyLog("operation arguments is undefined!");
        }
        if (opArgs.radioStation === undefined) {
            prettyLog("operation argument \"radioStation\" is undefined!");
        }
        favoriteStations.push(opArgs.radioStation);
        numberOfStationChanged();
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
        currentStationIndex %= favoriteStations.length;
        return false;
    }
};
