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

#include "joynr/vehicle/RadioProvider.h"
#include "joynr/vehicle/RadioStation.h"
#include "joynr/types/ProviderQos.h"
#include "joynr/joynrlogging.h"
#include <QList>
#include <QMutex>

/**
  * A Radio Provider with a circular list of radio stations
  */
class MyRadioProvider : public joynr::vehicle::RadioProvider
{
public:
    MyRadioProvider(const joynr::types::ProviderQos& providerQos);
    ~MyRadioProvider();

    /**
      * Get the current radio station
      */
    void getCurrentStation(joynr::RequestStatus& status, joynr::vehicle::RadioStation& result);

    /**
      * Set the current radio station
      */
    void setCurrentStation(joynr::RequestStatus& status,
                           joynr::vehicle::RadioStation currentStation);

    /**
      * Get the next radio station in a circular list of stations
      */
    void shuffleStations(joynr::RequestStatus& status);

    /**
      * Add a favourite radio station
      */
    void addFavouriteStation(joynr::RequestStatus& status,
                             bool& returnValue,
                             joynr::vehicle::RadioStation radioStation);

private:
    // Disallow copy and assign
    MyRadioProvider(const MyRadioProvider&);
    void operator=(const MyRadioProvider&);

    int currentStationIndex;                          // Index to the current station
    QList<joynr::vehicle::RadioStation> stationsList; // List of possible stations
    QMutex mutex;                                     // Providers need to be threadsafe

    static joynr::joynr_logging::Logger* logger;
};

#endif
