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
#ifndef MY_RADIO_PROVIDER_H
#define MY_RADIO_PROVIDER_H

#include "joynr/vehicle/DefaultRadioProvider.h"
#include "joynr/vehicle/QtRadioStation.h"
#include "joynr/vehicle/QtCountry.h"
#include "joynr/joynrlogging.h"
#include <QList>
#include <QMap>
#include <QMutex>

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
    void getCurrentStation(std::function<
            void(const joynr::vehicle::RadioTypes::StdRadioStation& result)> onSuccess);

    /**
      * Get the next radio station in a circular list of stations
      */
    void shuffleStations(std::function<void()> onSuccess);

    /**
      * Add a favourite radio station
      */
    void addFavouriteStation(const joynr::vehicle::RadioTypes::StdRadioStation& radioStation,
                             std::function<void(const bool& returnValue)> onSuccess);

    void fireWeakSignalBroadcast();
    void fireNewStationDiscoveredBroadcast();
    void getLocationOfCurrentStation(std::function<
            void(const joynr::vehicle::RadioTypes::StdCountry::Enum& country,
                 const joynr::vehicle::RadioTypes::StdGeoPosition& location)> onSuccess);

private:
    // Disallow copy and assign
    MyRadioProvider(const MyRadioProvider&);
    void operator=(const MyRadioProvider&);

    int currentStationIndex;                                         // Index to the current station
    QList<joynr::vehicle::RadioTypes::StdRadioStation> stationsList; // List of possible stations
    QMap<joynr::vehicle::RadioTypes::StdCountry::Enum, joynr::vehicle::RadioTypes::StdGeoPosition>
            countryGeoPositionMap;
    QMutex mutex; // Providers need to be threadsafe

    static joynr::joynr_logging::Logger* logger;
};

#endif
