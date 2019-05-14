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

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.ReplyContext;
import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.UsedBy;

import joynr.examples.statelessasync.VehicleConfiguration;
import joynr.examples.statelessasync.VehicleState.GetCurrentConfigErrorEnum;

@UsedBy(VehicleStateProxy.class)
public interface VehicleStateStatelessAsyncCallback extends StatelessAsyncCallback {

	/*
	* numberOfConfigs getter
	*/
	@StatelessCallbackCorrelation("-2105526245")
	default void getNumberOfConfigsSuccess(Integer numberOfConfigs, ReplyContext replyContext)
	{ throw new UnsupportedOperationException("getNumberOfConfigsSuccess not implemented for callback instance"); }
	@StatelessCallbackCorrelation("-2105526245")
	default void getNumberOfConfigsFailed(
			JoynrRuntimeException runtimeException,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("getNumberOfConfigsFailed not implemented for callback instance"); }

	/*
	* getCurrentConfig
	*/
	@StatelessCallbackCorrelation("-875443412")
	default void getCurrentConfigSuccess(
			VehicleConfiguration result,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("getCurrentConfigSuccess not implemented for callback instance"); }
	default void getCurrentConfigFailed(
			joynr.examples.statelessasync.VehicleState.GetCurrentConfigErrorEnum error,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("getCurrentConfigFailed with error not implemented for callback instance"); }
	default void getCurrentConfigFailed(
			JoynrRuntimeException runtimeException,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("getCurrentConfigFailed with exception not implemented for callback instance"); }

	/*
	* addConfiguration
	*/
	@StatelessCallbackCorrelation("1869609429")
	default void addConfigurationSuccess(
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("addConfigurationSuccess not implemented for callback instance"); }
	default void addConfigurationFailed(
			JoynrRuntimeException runtimeException,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("addConfigurationFailed with exception not implemented for callback instance"); }

	/*
	* callWithExceptionTest
	*/
	@StatelessCallbackCorrelation("-50182243")
	default void callWithExceptionTestSuccess(
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("callWithExceptionTestSuccess not implemented for callback instance"); }
	default void callWithExceptionTestFailed(
			JoynrRuntimeException runtimeException,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("callWithExceptionTestFailed with exception not implemented for callback instance"); }
}
