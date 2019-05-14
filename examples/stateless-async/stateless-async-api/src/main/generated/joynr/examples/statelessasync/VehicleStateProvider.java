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

import io.joynr.provider.Promise;
import io.joynr.provider.Deferred;
import io.joynr.provider.AbstractDeferred;
import io.joynr.provider.DeferredVoid;
import joynr.exceptions.ApplicationException;

import io.joynr.provider.JoynrInterface;
import io.joynr.JoynrVersion;

import joynr.examples.statelessasync.VehicleConfiguration;

import io.joynr.provider.SubscriptionPublisherInjection;

interface VehicleStateSubscriptionPublisherInjection extends SubscriptionPublisherInjection<VehicleStateSubscriptionPublisher> {}

@JoynrInterface(provides = VehicleState.class, provider = VehicleStateProvider.class, name = "examples/statelessasync/VehicleState")
@JoynrVersion(major = 0, minor = 1)
public interface VehicleStateProvider extends VehicleStateSubscriptionPublisherInjection {


	Promise<Deferred<Integer>> getNumberOfConfigs();

	/**
	 * getCurrentConfig
	 * @param id the parameter id
	 * @return promise for asynchronous handling
	 */
	public Promise<GetCurrentConfigDeferred> getCurrentConfig(
			String id
	);

	/**
	 * addConfiguration
	 * @param configuration the parameter configuration
	 * @return promise for asynchronous handling
	 */
	public Promise<DeferredVoid> addConfiguration(
			VehicleConfiguration configuration
	);

	/**
	 * callWithExceptionTest
	 * @param addToExceptionMessage the parameter addToExceptionMessage
	 * @return promise for asynchronous handling
	 */
	public Promise<DeferredVoid> callWithExceptionTest(
			String addToExceptionMessage
	);

	/**
	 * callFireAndForgetTest
	 * @param dataIn the parameter dataIn
	 * @return promise for asynchronous handling
	 */
	public void callFireAndForgetTest(
			String dataIn
	);

	public class GetCurrentConfigDeferred extends AbstractDeferred {
		public synchronized boolean resolve(VehicleConfiguration result) {
			return super.resolve(result);
		}
		public synchronized boolean reject(joynr.examples.statelessasync.VehicleState.GetCurrentConfigErrorEnum error) {
			return super.reject(new ApplicationException(error));
		}
	}
}
