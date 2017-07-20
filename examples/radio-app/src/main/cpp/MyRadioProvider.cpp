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

#include "MyRadioProvider.h"

#include <chrono>

#include "MyRadioHelper.h"

using namespace joynr;

INIT_LOGGER(MyRadioProvider);

MyRadioProvider::MyRadioProvider()
        : DefaultRadioProvider(),
          currentStationIndex(0),
          stationsList(),
          countryGeoPositionMap(),
          mutex()
{
    stationsList.push_back(
            vehicle::RadioStation("ABC Trible J", true, vehicle::Country::AUSTRALIA));
    stationsList.push_back(vehicle::RadioStation("Radio Popolare", false, vehicle::Country::ITALY));
    stationsList.push_back(vehicle::RadioStation("JAZZ.FM91", false, vehicle::Country::CANADA));
    stationsList.push_back(vehicle::RadioStation("Bayern 3", true, vehicle::Country::GERMANY));
    countryGeoPositionMap.insert({vehicle::Country::AUSTRALIA,
                                  vehicle::GeoPosition(-37.8141070, 144.9632800)}); // Melbourne
    countryGeoPositionMap.insert(
            {vehicle::Country::ITALY, vehicle::GeoPosition(46.4982950, 11.3547580)}); // Bolzano
    countryGeoPositionMap.insert(
            {vehicle::Country::CANADA, vehicle::GeoPosition(53.5443890, -113.4909270)}); // Edmonton
    countryGeoPositionMap.insert(
            {vehicle::Country::GERMANY, vehicle::GeoPosition(48.1351250, 11.5819810)}); // Munich
    currentStation = stationsList.at(currentStationIndex);
}

MyRadioProvider::~MyRadioProvider()
{
}

void MyRadioProvider::getCurrentStation(
        std::function<void(const vehicle::RadioStation&)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::lock_guard<std::mutex> locker(mutex);
    std::ignore = onError;
    MyRadioHelper::prettyLog(logger, "getCurrentStation -> " + currentStation.toString());
    onSuccess(currentStation);
}

void MyRadioProvider::shuffleStations(
        std::function<void()> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::lock_guard<std::mutex> locker(mutex);

    std::ignore = onError;
    vehicle::RadioStation oldStation = currentStation;
    ++currentStationIndex;
    currentStationIndex %= stationsList.size();
    currentStationChanged(stationsList.at(currentStationIndex));
    currentStation = stationsList.at(currentStationIndex);
    MyRadioHelper::prettyLog(
            logger,
            "shuffleStations: " + oldStation.toString() + " -> " + currentStation.toString());
    onSuccess();
}

void MyRadioProvider::addFavoriteStation(
        const vehicle::RadioStation& radioStation,
        std::function<void(const bool& returnValue)> onSuccess,
        std::function<void(const joynr::vehicle::Radio::AddFavoriteStationErrorEnum::Enum&)>
                onError)
{
    std::lock_guard<std::mutex> locker(mutex);

    if (radioStation.getName().empty()) {
        throw joynr::exceptions::ProviderRuntimeException(MyRadioHelper::MISSING_NAME());
    }

    bool duplicateFound = false;
    for (joynr::vehicle::RadioStation station : stationsList) {
        if (!duplicateFound && station.getName() == radioStation.getName()) {
            duplicateFound = true;
            onError(joynr::vehicle::Radio::AddFavoriteStationErrorEnum::DUPLICATE_RADIOSTATION);
            break;
        }
    }
    if (!duplicateFound) {
        MyRadioHelper::prettyLog(logger, "addFavoriteStation(" + radioStation.toString() + ")");
        stationsList.push_back(radioStation);
        onSuccess(true);
    }
}

void MyRadioProvider::getLocationOfCurrentStation(
        std::function<void(const joynr::vehicle::Country::Enum& country,
                           const joynr::vehicle::GeoPosition& location)> onSuccess,
        std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
{
    std::ignore = onError;
    joynr::vehicle::Country::Enum country(currentStation.getCountry());
    joynr::vehicle::GeoPosition location(countryGeoPositionMap.at(country));
    MyRadioHelper::prettyLog(
            logger,
            "getLocationOfCurrentStation: return country \"" +
                    joynr::vehicle::Country::getLiteral(currentStation.getCountry()) +
                    "\" and location \"" + location.toString() + "\"");
    onSuccess(country, location);
}

void MyRadioProvider::fireWeakSignalBroadcast()
{
    MyRadioHelper::prettyLog(logger, "fire weakSignalBroadcast: " + currentStation.toString());
    fireWeakSignal(currentStation);
}

void MyRadioProvider::fireNewStationDiscoveredBroadcast()
{
    vehicle::RadioStation discoveredStation(stationsList.at(currentStationIndex));
    vehicle::GeoPosition geoPosition(countryGeoPositionMap.at(discoveredStation.getCountry()));
    MyRadioHelper::prettyLog(logger,
                             "fire newStationDiscoveredBroadcast: " + discoveredStation.toString() +
                                     " at " + geoPosition.toString());
    fireNewStationDiscovered(discoveredStation, geoPosition);
}
