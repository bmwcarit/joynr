/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "MyRadioHelper.h"
#include "joynr/RequestStatus.h"

using namespace joynr;

joynr_logging::Logger* MyRadioProvider::logger =
        joynr_logging::Logging::getInstance()->getLogger("DEMO", "MyRadioProvider");

MyRadioProvider::MyRadioProvider(const types::ProviderQos& providerQos)
        : RadioProvider(providerQos),
          currentStationIndex(0),
          stationsList(),
          countryGeoPositionMap(),
          mutex()
{
    stationsList << vehicle::RadioStation("ABC Trible J", true, vehicle::Country::AUSTRALIA)
                 << vehicle::RadioStation("Radio Popolare", false, vehicle::Country::ITALY)
                 << vehicle::RadioStation("JAZZ.FM91", false, vehicle::Country::CANADA)
                 << vehicle::RadioStation("Bayern 3", true, vehicle::Country::GERMANY);
    countryGeoPositionMap.insert(vehicle::Country::AUSTRALIA,
                                 vehicle::GeoPosition(-37.8141070, 144.9632800)); // Melbourne
    countryGeoPositionMap.insert(
            vehicle::Country::ITALY, vehicle::GeoPosition(46.4982950, 11.3547580)); // Bolzano
    countryGeoPositionMap.insert(
            vehicle::Country::CANADA, vehicle::GeoPosition(53.5443890, -113.4909270)); // Edmonton
    countryGeoPositionMap.insert(
            vehicle::Country::GERMANY, vehicle::GeoPosition(48.1351250, 11.5819810)); // Munich
    currentStation = stationsList.at(currentStationIndex);
}

MyRadioProvider::~MyRadioProvider()
{
}

void MyRadioProvider::getCurrentStation(
        std::function<void(const joynr::RequestStatus& status,
                           const joynr::vehicle::RadioStation& result)> callbackFct)
{
    QMutexLocker locker(&mutex);

    MyRadioHelper::prettyLog(
            logger, QString("getCurrentStation -> %1").arg(currentStation.toString()));
    callbackFct(RequestStatus(RequestStatusCode::OK), currentStation);
}

void MyRadioProvider::shuffleStations(
        std::function<void(const joynr::RequestStatus& status)> callbackFct)
{
    QMutexLocker locker(&mutex);

    vehicle::RadioStation oldStation = currentStation;
    ++currentStationIndex;
    currentStationIndex %= stationsList.size();
    currentStationChanged(stationsList.at(currentStationIndex));
    MyRadioHelper::prettyLog(logger,
                             QString("shuffleStations: %1 -> %2").arg(oldStation.toString()).arg(
                                     currentStation.toString()));
    callbackFct(RequestStatus(RequestStatusCode::OK));
}

void MyRadioProvider::addFavouriteStation(const vehicle::RadioStation& radioStation,
                                          std::function<void(const joynr::RequestStatus& status,
                                                             const bool& returnValue)> callbackFct)
{
    QMutexLocker locker(&mutex);

    MyRadioHelper::prettyLog(
            logger, QString("addFavouriteStation(%1)").arg(radioStation.toString()));
    stationsList.append(radioStation);
    callbackFct(RequestStatus(RequestStatusCode::OK), true);
}

void MyRadioProvider::getLocationOfCurrentStation(
        std::function<void(const joynr::RequestStatus& joynrInternalStatus,
                           const joynr::vehicle::Country::Enum& country,
                           const joynr::vehicle::GeoPosition& location)> callbackFct)
{
    joynr::vehicle::Country::Enum country(currentStation.getCountry());
    joynr::vehicle::GeoPosition location(countryGeoPositionMap.value(country));
    MyRadioHelper::prettyLog(
            logger,
            QString("getLocationOfCurrentStation: return country \"%1\" and location \"%2\"")
                    .arg(currentStation.getCountryInternal())
                    .arg(location.toString()));
    callbackFct(RequestStatus(RequestStatusCode::OK), country, location);
}

void MyRadioProvider::fireWeakSignalBroadcast()
{
    MyRadioHelper::prettyLog(
            logger, QString("fire weakSignalBroadacst: %1").arg(currentStation.toString()));
    fireWeakSignal(currentStation);
}

void MyRadioProvider::fireNewStationDiscoveredBroadcast()
{
    vehicle::RadioStation discoveredStation(stationsList.at(currentStationIndex));
    vehicle::GeoPosition geoPosition(countryGeoPositionMap.value(discoveredStation.getCountry()));
    MyRadioHelper::prettyLog(logger,
                             QString("fire newStationDiscoveredBroadcast: %1 at %2")
                                     .arg(discoveredStation.toString())
                                     .arg(geoPosition.toString()));
    fireNewStationDiscovered(discoveredStation, geoPosition);
}

void MyRadioProvider::setCurrentStation(
        vehicle::RadioStation currentStation,
        std::function<void(const joynr::RequestStatus& status)> callbackFct)
{
    QMutexLocker locker(&mutex);

    int index = stationsList.indexOf(currentStation);

    if (index == -1) {
        // the station is not known
        stationsList.append(currentStation);
        index = stationsList.indexOf(currentStation);
    }

    currentStationIndex = index;
    currentStationChanged(stationsList.at(currentStationIndex));
    MyRadioHelper::prettyLog(
            logger, QString("setCurrentStation(%1)").arg(currentStation.toString()));
    callbackFct(RequestStatus(RequestStatusCode::OK));
}
