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

joynr_logging::Logger* MyRadioProvider::logger = joynr_logging::Logging::getInstance()->getLogger("DEMO", "MyRadioProvider");

MyRadioProvider::MyRadioProvider(const types::ProviderQos& providerQos) :
    RadioProvider(providerQos),
    currentStationIndex(0),
    stationsList(),
    mutex()
{
    stationsList << QString("Triple J") << QString("FM 4") << QString("Radio ABC");
    isOn = false;
    numberOfStations = stationsList.size();
    currentStation = stationsList.at(currentStationIndex);
}

MyRadioProvider::~MyRadioProvider()
{
}

void MyRadioProvider::getCurrentStation(RequestStatus& status, QString& result)
{
    QMutexLocker locker(&mutex);

    result = currentStation;
    MyRadioHelper::prettyLog(logger, QString("getCurrentStation -> %1").arg(result));
    status.setCode(RequestStatusCode::OK);
}

void MyRadioProvider::shuffleStations(RequestStatus& status)
{
    QMutexLocker locker(&mutex);

    QString oldStation = currentStation;
    ++currentStationIndex;
    currentStationIndex %= stationsList.size();
    currentStationChanged(stationsList.at(currentStationIndex));
    MyRadioHelper::prettyLog(logger, QString("shuffleStations: %1 -> %2").arg(oldStation).arg(currentStation));
    status.setCode(RequestStatusCode::OK);
}

void MyRadioProvider::addFavouriteStation(RequestStatus& status, bool& returnValue, QString radioStation)
{
    QMutexLocker locker(&mutex);

    MyRadioHelper::prettyLog(logger, QString("addFavouriteStation(%1)").arg(radioStation));
    stationsList.append(radioStation);
    numberOfStationsChanged(stationsList.size());
    returnValue = true;
    status.setCode(RequestStatusCode::OK);
}

void MyRadioProvider::setCurrentStation(RequestStatus& status, QString currentStation)
{
    QMutexLocker locker(&mutex);

    int index = stationsList.indexOf(currentStation);

    if(index == -1) {
        // the station is not known
        stationsList.append(currentStation);
        index = stationsList.indexOf(currentStation);
        numberOfStationsChanged(stationsList.size());
    }

    currentStationIndex = index;
    currentStationChanged(stationsList.at(currentStationIndex));
    MyRadioHelper::prettyLog(logger, QString("setCurrentStation(%1)").arg(currentStation));
    status.setCode(RequestStatusCode::OK);
}

void MyRadioProvider::addFavouriteStationList(RequestStatus& status, bool& returnValue, QList<QString>  radioStationList)
{
    foreach (const QString& station, radioStationList) {
        addFavouriteStation(status, returnValue, station);
    }
}

