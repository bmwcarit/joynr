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
#ifndef MY_RADIO_PROVIDER_H
#define MY_RADIO_PROVIDER_H

#include "joynr/vehicle/DefaultRadioProvider.h"
#include "joynr/vehicle/RadioStation.h"
#include "joynr/vehicle/Country.h"
#include "joynr/Logger.h"
#include "joynr/exceptions/JoynrException.h"
#include <vector>
#include <unordered_map>
#include <mutex>

using namespace joynr;
/**
  * A Radio Provider with a circular list of radio stations
  */
class MyRadioProvider : public joynr::vehicle::DefaultRadioProvider
{
public:
    MyRadioProvider();
    ~MyRadioProvider();

    /**
      * Get the current radio station
      */
    void getCurrentStation(
            std::function<void(const joynr::vehicle::RadioStation&)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError);

    /**
      * Get the next radio station in a circular list of stations
      */
    void shuffleStations(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError);

    /**
      * Add a favorite radio station
      */
    void addFavoriteStation(
            const joynr::vehicle::RadioStation& newFavoriteStation,
            std::function<void(const bool& success)> onSuccess,
            std::function<void(const joynr::vehicle::Radio::AddFavoriteStationErrorEnum::Enum&
                                       errorEnum)> onError);

    void getLocationOfCurrentStation(
            std::function<void(const joynr::vehicle::Country::Enum& country,
                               const joynr::vehicle::GeoPosition& location)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError);

    void fireWeakSignalBroadcast();
    void fireNewStationDiscoveredBroadcast();

private:
    // Disallow copy and assign
    MyRadioProvider(const MyRadioProvider&);
    void operator=(const MyRadioProvider&);

    int currentStationIndex;                                // Index to the current station
    std::vector<joynr::vehicle::RadioStation> stationsList; // List of possible stations
    std::unordered_map<joynr::vehicle::Country::Enum, joynr::vehicle::GeoPosition>
            countryGeoPositionMap;
    std::mutex mutex; // Providers need to be threadsafe

    ADD_LOGGER(MyRadioProvider);
};

#endif
