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

package joynr.test;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.ReplyContext;
import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.UsedBy;


@UsedBy(SystemIntegrationTestProxy.class)
public interface SystemIntegrationTestStatelessAsyncCallback extends StatelessAsyncCallback {


	/*
	* add
	*/
	@StatelessCallbackCorrelation("986573632")
	default void addSuccess(
			Integer result,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("addSuccess not implemented for callback instance"); }
	default void addFailed(
			JoynrRuntimeException runtimeException,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("addFailed with exception not implemented for callback instance"); }
}
