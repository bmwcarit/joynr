/*
 *
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
 *
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
 */

// #####################################################
//#######################################################
//###                                                 ###
//##    WARNING: This file is generated. DO NOT EDIT   ##
//##             All changes will be lost!             ##
//###                                                 ###
//#######################################################
// #####################################################

package joynr.examples.statelessasync;

import io.joynr.messaging.MessagingQos;
import io.joynr.Sync;
import io.joynr.ProvidedBy;
import io.joynr.UsedBy;
import joynr.exceptions.ApplicationException;

import joynr.examples.statelessasync.VehicleConfiguration;

@Sync
@ProvidedBy(VehicleStateProvider.class)
@UsedBy(VehicleStateProxy.class)
public interface VehicleStateSync extends VehicleState, VehicleStateFireAndForget {

	public Integer getNumberOfConfigs();
	default public Integer getNumberOfConfigs(MessagingQos messagingQos) {
		return getNumberOfConfigs();
	}


	/*
	* getCurrentConfig
	*/
	public VehicleConfiguration getCurrentConfig(
			String id
	) throws ApplicationException;
	default public VehicleConfiguration getCurrentConfig(
			String id,
			MessagingQos messagingQos
	) throws ApplicationException {
		return getCurrentConfig(
			id
		);
	}

	/*
	* addConfiguration
	*/
	public void addConfiguration(
			VehicleConfiguration configuration
	);
	default public void addConfiguration(
			VehicleConfiguration configuration,
			MessagingQos messagingQos
	) {
		return;
	}

	/*
	* callWithExceptionTest
	*/
	public void callWithExceptionTest(
			String addToExceptionMessage
	);
	default public void callWithExceptionTest(
			String addToExceptionMessage,
			MessagingQos messagingQos
	) {
		return;
	}
}
