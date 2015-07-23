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

#include <QDateTime>

#include "MyRadioHelper.h"
#include "joynr/RequestStatus.h"

using namespace joynr;

joynr_logging::Logger* MyRadioProvider::logger =
        joynr_logging::Logging::getInstance()->getLogger("DEMO", "MyRadioProvider");

MyRadioProvider::MyRadioProvider()
        : DefaultRadioProvider(),
          currentStationIndex(0),
          stationsList(),
          countryGeoPositionMap(),
          mutex()
{
    // Initialise the quality of service settings
    // Set the priority so that the consumer application always uses the most recently
    // started provider
    providerQos.setPriority(QDateTime::currentDateTime().toMSecsSinceEpoch());

    stationsList << vehicle::RadioTypes::StdRadioStation(
                            "ABC Trible J", true, vehicle::RadioTypes::StdCountry::AUSTRALIA)
                 << vehicle::RadioTypes::StdRadioStation(
                            "Radio Popolare", false, vehicle::RadioTypes::StdCountry::ITALY)
                 << vehicle::RadioTypes::StdRadioStation(
                            "JAZZ.FM91", false, vehicle::RadioTypes::StdCountry::CANADA)
                 << vehicle::RadioTypes::StdRadioStation(
                            "Bayern 3", true, vehicle::RadioTypes::StdCountry::GERMANY);
    countryGeoPositionMap.insert(
            vehicle::RadioTypes::StdCountry::AUSTRALIA,
            vehicle::RadioTypes::StdGeoPosition(-37.8141070, 144.9632800)); // Melbourne
    countryGeoPositionMap.insert(
            vehicle::RadioTypes::StdCountry::ITALY,
            vehicle::RadioTypes::StdGeoPosition(46.4982950, 11.3547580)); // Bolzano
    countryGeoPositionMap.insert(
            vehicle::RadioTypes::StdCountry::CANADA,
            vehicle::RadioTypes::StdGeoPosition(53.5443890, -113.4909270)); // Edmonton
    countryGeoPositionMap.insert(
            vehicle::RadioTypes::StdCountry::GERMANY,
            vehicle::RadioTypes::StdGeoPosition(48.1351250, 11.5819810)); // Munich
    currentStation = stationsList.at(currentStationIndex);
}

MyRadioProvider::~MyRadioProvider()
{
}

void MyRadioProvider::getCurrentStation(
        std::function<void(const vehicle::RadioTypes::StdRadioStation&)> onSuccess)
{
    QMutexLocker locker(&mutex);

    MyRadioHelper::prettyLog(logger,
                             QString("getCurrentStation -> %1")
                                     .arg(QString::fromStdString(currentStation.toString())));
    onSuccess(currentStation);
}

void MyRadioProvider::shuffleStations(std::function<void()> onSuccess)
{
    QMutexLocker locker(&mutex);

    vehicle::RadioTypes::StdRadioStation oldStation = currentStation;
    ++currentStationIndex;
    currentStationIndex %= stationsList.size();
    currentStationChanged(stationsList.at(currentStationIndex));
    MyRadioHelper::prettyLog(logger,
                             QString("shuffleStations: %1 -> %2")
                                     .arg(QString::fromStdString(oldStation.toString()))
                                     .arg(QString::fromStdString(currentStation.toString())));
    onSuccess();
}

void MyRadioProvider::addFavouriteStation(const vehicle::RadioTypes::StdRadioStation& radioStation,
                                          std::function<void(const bool& returnValue)> onSuccess)
{
    QMutexLocker locker(&mutex);

    MyRadioHelper::prettyLog(logger,
                             QString("addFavouriteStation(%1)")
                                     .arg(QString::fromStdString(radioStation.toString())));
    stationsList.append(radioStation);
    onSuccess(true);
}

void MyRadioProvider::getLocationOfCurrentStation(
        std::function<void(const joynr::vehicle::RadioTypes::StdCountry::Enum& country,
                           const joynr::vehicle::RadioTypes::StdGeoPosition& location)> onSuccess)
{
    joynr::vehicle::RadioTypes::StdCountry::Enum country(currentStation.getCountry());
    joynr::vehicle::RadioTypes::StdGeoPosition location(countryGeoPositionMap.value(country));
    MyRadioHelper::prettyLog(
            logger,
            QString("getLocationOfCurrentStation: return country \"%1\" and location \"%2\"")
                    .arg(QString::fromStdString(joynr::vehicle::RadioTypes::StdCountry::getLiteral(
                            currentStation.getCountry())))
                    .arg(QString::fromStdString(location.toString())));
    onSuccess(country, location);
}

void MyRadioProvider::fireWeakSignalBroadcast()
{
    MyRadioHelper::prettyLog(logger,
                             QString("fire weakSignalBroadacst: %1")
                                     .arg(QString::fromStdString(currentStation.toString())));
    fireWeakSignal(currentStation);
}

void MyRadioProvider::fireNewStationDiscoveredBroadcast()
{
    vehicle::RadioTypes::StdRadioStation discoveredStation(stationsList.at(currentStationIndex));
    vehicle::RadioTypes::StdGeoPosition geoPosition(
            countryGeoPositionMap.value(discoveredStation.getCountry()));
    MyRadioHelper::prettyLog(logger,
                             QString("fire newStationDiscoveredBroadcast: %1 at %2")
                                     .arg(QString::fromStdString(discoveredStation.toString()))
                                     .arg(QString::fromStdString(geoPosition.toString())));
    fireNewStationDiscovered(discoveredStation, geoPosition);
}
