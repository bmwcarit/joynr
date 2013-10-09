package io.joynr.demo;

/*
 * #%L
 * joynr::demos::radio-app
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
import io.joynr.dispatcher.rpc.annotation.JoynrRpcParam;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcReturn;
import io.joynr.exceptions.JoynrArbitrationException;

import java.util.List;

import joynr.vehicle.RadioAbstractProvider;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.google.common.collect.Lists;

public class MyRadioProvider extends RadioAbstractProvider {
    private static final String PRINT_BORDER = "\n####################\n";
    private static final Logger LOG = LoggerFactory.getLogger(MyRadioProvider.class);

    private List<String> stationsList = Lists.newArrayList("Triple J", "FM 4", "Radio ABC");

    private int notSoRandomCounter = 0;

    public MyRadioProvider() {
        isOn = Boolean.FALSE;
        providerQos.setPriority(System.currentTimeMillis());
        numberOfStations = stationsList.size();
        currentStation = stationsList.get(notSoRandomCounter);
    }

    @Override
    public String getCurrentStation() throws JoynrArbitrationException {
        LOG.debug(PRINT_BORDER + "getCurrentSation -> " + currentStation + PRINT_BORDER);
        return currentStation;
    }

    @Override
    public void shuffleStations() throws JoynrArbitrationException {
        String oldStation = currentStation;
        notSoRandomCounter++;
        notSoRandomCounter = notSoRandomCounter % stationsList.size();
        currentStationChanged(stationsList.get(notSoRandomCounter));
        LOG.debug(PRINT_BORDER + "shuffleStations: " + oldStation + " -> " + currentStation + PRINT_BORDER);
    }

    @Override
    public Boolean getIsOn() {
        LOG.debug(PRINT_BORDER + "getIsOn -> " + isOn + PRINT_BORDER);
        return isOn;
    }

    @Override
    public void setIsOn(Boolean isOn) {
        LOG.debug(PRINT_BORDER + "setIsOn(" + isOn + ")" + PRINT_BORDER);
        isOnChanged(isOn);
    }

    @Override
    @JoynrRpcReturn(deserialisationType = BooleanToken.class)
    public Boolean addFavouriteStation(@JoynrRpcParam("radioStation") String radioStation)
                                                                                         throws JoynrArbitrationException {
        LOG.debug(PRINT_BORDER + "addFavouriteStation(" + radioStation + ")" + PRINT_BORDER);
        stationsList.add(radioStation);
        numberOfStationsChanged(stationsList.size());
        return true;
    }

    @Override
    @JoynrRpcReturn(deserialisationType = BooleanToken.class)
    public Boolean addFavouriteStationList(@JoynrRpcParam("radioStationList") List<String> radioStationList)
                                                                                                           throws JoynrArbitrationException {
        LOG.debug(PRINT_BORDER + "addFavouriteStationList(" + radioStationList + ")" + PRINT_BORDER);
        stationsList.addAll(radioStationList);
        numberOfStationsChanged(stationsList.size());
        return true;
    }

	@Override
	public Integer getNumberOfStations() {
		return numberOfStations;
	}

	@Override
	public void setNumberOfStations(Integer numberOfStations) {
		numberOfStationsChanged(numberOfStations);
	}
}
