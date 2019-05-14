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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.provider.Promise;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;

import joynr.examples.statelessasync.VehicleConfiguration;

public class DefaultVehicleStateProvider extends VehicleStateAbstractProvider {
	private static final Logger logger = LoggerFactory.getLogger(DefaultVehicleStateProvider.class);

	protected Integer numberOfConfigs;

	public DefaultVehicleStateProvider() {
	}


	@Override
	public Promise<Deferred<Integer>> getNumberOfConfigs() {
		Deferred<Integer> deferred = new Deferred<>();
		deferred.resolve(numberOfConfigs);
		return new Promise<>(deferred);
	}


	/*
	* getCurrentConfig
	*/
	@Override
	public Promise<GetCurrentConfigDeferred> getCurrentConfig(
			String id) {
		logger.warn("**********************************************");
		logger.warn("* DefaultVehicleStateProvider.getCurrentConfig called");
		logger.warn("**********************************************");
		GetCurrentConfigDeferred deferred = new GetCurrentConfigDeferred();
		VehicleConfiguration result = new VehicleConfiguration();
		deferred.resolve(result);
		return new Promise<>(deferred);
	}

	/*
	* addConfiguration
	*/
	@Override
	public Promise<DeferredVoid> addConfiguration(
			VehicleConfiguration configuration) {
		logger.warn("**********************************************");
		logger.warn("* DefaultVehicleStateProvider.addConfiguration called");
		logger.warn("**********************************************");
		DeferredVoid deferred = new DeferredVoid();
		deferred.resolve();
		return new Promise<>(deferred);
	}

	/*
	* callWithExceptionTest
	*/
	@Override
	public Promise<DeferredVoid> callWithExceptionTest(
			String addToExceptionMessage) {
		logger.warn("**********************************************");
		logger.warn("* DefaultVehicleStateProvider.callWithExceptionTest called");
		logger.warn("**********************************************");
		DeferredVoid deferred = new DeferredVoid();
		deferred.resolve();
		return new Promise<>(deferred);
	}

	/*
	* callFireAndForgetTest
	*/
	@Override
	public void callFireAndForgetTest(
			String dataIn) {
		logger.warn("**********************************************");
		logger.warn("* DefaultVehicleStateProvider.callFireAndForgetTest called");
		logger.warn("**********************************************");
	}
}
