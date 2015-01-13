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

void MyRadioProvider::getCurrentStation(RequestStatus& status, vehicle::RadioStation& result)
{
    QMutexLocker locker(&mutex);

    result = currentStation;
    MyRadioHelper::prettyLog(logger, QString("getCurrentStation -> %1").arg(result.toString()));
    status.setCode(RequestStatusCode::OK);
}

void MyRadioProvider::shuffleStations(RequestStatus& status)
{
    QMutexLocker locker(&mutex);

    vehicle::RadioStation oldStation = currentStation;
    ++currentStationIndex;
    currentStationIndex %= stationsList.size();
    currentStationChanged(stationsList.at(currentStationIndex));
    MyRadioHelper::prettyLog(logger,
                             QString("shuffleStations: %1 -> %2").arg(oldStation.toString()).arg(
                                     currentStation.toString()));
    status.setCode(RequestStatusCode::OK);
}

void MyRadioProvider::addFavouriteStation(RequestStatus& status,
                                          bool& returnValue,
                                          vehicle::RadioStation radioStation)
{
    QMutexLocker locker(&mutex);

    MyRadioHelper::prettyLog(
            logger, QString("addFavouriteStation(%1)").arg(radioStation.toString()));
    stationsList.append(radioStation);
    returnValue = true;
    status.setCode(RequestStatusCode::OK);
}

void MyRadioProvider::fireWeakSignalEvent()
{
    MyRadioHelper::prettyLog(
            logger, QString("fire weakSignalEvent: %1").arg(currentStation.toString()));
    weakSignalEventOccurred(currentStation);
}

void MyRadioProvider::fireNewStationDiscoveredEvent()
{
    vehicle::RadioStation discoveredStation(stationsList.at(currentStationIndex));
    vehicle::GeoPosition geoPosition(countryGeoPositionMap.value(discoveredStation.getCountry()));
    MyRadioHelper::prettyLog(logger,
                             QString("fire newStationDiscoveredEvent: %1 at %2")
                                     .arg(discoveredStation.toString())
                                     .arg(geoPosition.toString()));
    newStationDiscoveredEventOccurred(discoveredStation, geoPosition);
}

void MyRadioProvider::setCurrentStation(RequestStatus& status, vehicle::RadioStation currentStation)
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
    status.setCode(RequestStatusCode::OK);
}
