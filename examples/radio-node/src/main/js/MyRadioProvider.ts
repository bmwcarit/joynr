/*
 * #%L
 * %%
 * Copyright (C) 2019 BMW Car IT GmbH
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

import { RadioProviderImplementation } from "../generated/js/joynr/vehicle/RadioProvider";
import * as RadioProvider from "../generated/js/joynr/vehicle/RadioProvider";
import {
    AddFavoriteStationReturns1,
    GetLocationOfCurrentStationReturns1
} from "../generated/js/joynr/vehicle/RadioProxy";
import { prettyLog } from "./logging";

import joynr = require("joynr");
import RadioStation = require("../generated/js/joynr/vehicle/RadioStation");
import Country = require("../generated/js/joynr/vehicle/Country");
import GeoPosition from "../generated/js/joynr/vehicle/GeoPosition";
import AddFavoriteStationErrorEnum from "../generated/js/joynr/vehicle/Radio/AddFavoriteStationErrorEnum";

class MyRadioProvider implements RadioProviderImplementation {
    public currentStation: any;
    private stationsList: RadioStation[];
    private myRadioProvider!: RadioProvider;
    private countryGeoPositionMap: Record<string, GeoPosition>;
    private currentStationIndex: number = 0;
    public constructor() {
        this.countryGeoPositionMap = {
            // Melbourne
            AUSTRALIA: new GeoPosition({
                latitude: -37.814107,
                longitude: 144.96328
            }),
            // Bolzano
            ITALY: new GeoPosition({
                latitude: 46.498295,
                longitude: 11.354758
            }),
            // Edmonton
            CANADA: new GeoPosition({
                latitude: 53.544389,
                longitude: -113.490927
            }),
            // Munich
            GERMANY: new GeoPosition({
                latitude: 48.135125,
                longitude: 11.581981
            })
        };

        this.stationsList = [
            new RadioStation({
                name: "ABC Trible J",
                trafficService: false,
                country: Country.AUSTRALIA
            }),
            new RadioStation({
                name: "Radio Popolare",
                trafficService: true,
                country: Country.ITALY
            }),
            new RadioStation({
                name: "JAZZ.FM91",
                trafficService: false,
                country: Country.CANADA
            }),
            new RadioStation({
                name: "Bayern 3",
                trafficService: true,
                country: Country.GERMANY
            })
        ];

        this.currentStation = {
            get() {
                prettyLog("radioProvider.currentStation.get() called");
                return this.stationsList[this.currentStationIndex];
            }
        };

        this.shuffleStations = this.shuffleStations.bind(this);
        this.getLocationOfCurrentStation = this.getLocationOfCurrentStation.bind(this);
        this.addFavoriteStation = this.addFavoriteStation.bind(this);
        this.currentStation.get = this.currentStation.get.bind(this);
    }

    public setProvider(radioProvider: RadioProvider): void {
        this.myRadioProvider = radioProvider;
    }

    public addFavoriteStation(opArgs: any): AddFavoriteStationReturns1 {
        prettyLog(`radioProvider.addFavoriteStation(${JSON.stringify(opArgs)}) called`);

        if (opArgs === undefined) {
            prettyLog("operation arguments is undefined!");
            return {
                success: false
            };
        }
        if (opArgs.newFavoriteStation === undefined) {
            prettyLog('operation argument "newFavoriteStation" is undefined!');
            return {
                success: false
            };
        }
        if (opArgs.newFavoriteStation.name === "") {
            throw new joynr.exceptions.ProviderRuntimeException({
                detailMessage: "MISSING_NAME"
            });
        }

        let i;
        // copy over and type each single member
        for (i in this.stationsList) {
            if (this.stationsList.hasOwnProperty(i)) {
                if (this.stationsList[i].name === opArgs.newFavoriteStation.name) {
                    throw AddFavoriteStationErrorEnum.DUPLICATE_RADIOSTATION;
                }
            }
        }

        this.stationsList.push(opArgs.newFavoriteStation);
        return {
            success: true
        };
    }

    public shuffleStations(): void {
        prettyLog(`radioProvider.shuffleStations() called`);
        this.currentStationIndex++;
        this.currentStationIndex %= this.stationsList.length;

        this.currentStation.valueChanged(this.stationsList[this.currentStationIndex]);
    }

    public fireWeakSignal(): void {
        const broadcast = this.myRadioProvider.weakSignal;
        const outputParams = broadcast.createBroadcastOutputParameters();
        outputParams.setWeakSignalStation(this.stationsList[this.currentStationIndex]);
        broadcast.fire(outputParams);
    }

    public fireWeakSignalWithPartition(): void {
        const broadcast = this.myRadioProvider.weakSignal;
        const outputParams = broadcast.createBroadcastOutputParameters();
        const currentStation = this.stationsList[this.currentStationIndex];
        outputParams.setWeakSignalStation(currentStation);
        broadcast.fire(outputParams, [currentStation.country.name]);
    }

    public getLocationOfCurrentStation(): GetLocationOfCurrentStationReturns1 {
        prettyLog("radioProvider.getLocationOfCurrentStation called");
        return {
            country: this.stationsList[this.currentStationIndex].country,
            location: this.countryGeoPositionMap[this.stationsList[this.currentStationIndex].country.name]
        };
    }
}

export = MyRadioProvider;
