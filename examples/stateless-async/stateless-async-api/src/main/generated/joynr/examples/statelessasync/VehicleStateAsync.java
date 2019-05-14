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
import io.joynr.proxy.Callback;
import io.joynr.proxy.ICallbackWithModeledError;
import io.joynr.proxy.CallbackWithModeledError;
import io.joynr.proxy.Future;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.Async;
import io.joynr.ProvidedBy;
import io.joynr.UsedBy;

import joynr.examples.statelessasync.VehicleConfiguration;


@Async
@ProvidedBy(VehicleStateProvider.class)
@UsedBy(VehicleStateProxy.class)
public interface VehicleStateAsync extends VehicleState, VehicleStateFireAndForget {

	public Future<Integer> getNumberOfConfigs(@JoynrRpcCallback(deserializationType = Integer.class) Callback<Integer> callback);
	default public Future<Integer> getNumberOfConfigs(@JoynrRpcCallback(deserializationType = Integer.class) Callback<Integer> callback, MessagingQos messagingQos) {
		return getNumberOfConfigs(callback);
	}




	/*
	* getCurrentConfig
	*/
	public Future<VehicleConfiguration> getCurrentConfig(
			@JoynrRpcCallback(deserializationType = VehicleConfiguration.class) CallbackWithModeledError<VehicleConfiguration,joynr.examples.statelessasync.VehicleState.GetCurrentConfigErrorEnum> callback,
			String id
	);
	default public Future<VehicleConfiguration> getCurrentConfig(
			@JoynrRpcCallback(deserializationType = VehicleConfiguration.class) CallbackWithModeledError<VehicleConfiguration,joynr.examples.statelessasync.VehicleState.GetCurrentConfigErrorEnum> callback,
			String id,
			MessagingQos messagingQos
	) {
		return getCurrentConfig(
			callback,
			id
		);
	}


	/*
	* addConfiguration
	*/
	public Future<Void> addConfiguration(
			@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback,
			VehicleConfiguration configuration
	);
	default public Future<Void> addConfiguration(
			@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback,
			VehicleConfiguration configuration,
			MessagingQos messagingQos
	) {
		return addConfiguration(
			callback,
			configuration
		);
	}


	/*
	* callWithExceptionTest
	*/
	public Future<Void> callWithExceptionTest(
			@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback,
			String addToExceptionMessage
	);
	default public Future<Void> callWithExceptionTest(
			@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback,
			String addToExceptionMessage,
			MessagingQos messagingQos
	) {
		return callWithExceptionTest(
			callback,
			addToExceptionMessage
		);
	}

}
