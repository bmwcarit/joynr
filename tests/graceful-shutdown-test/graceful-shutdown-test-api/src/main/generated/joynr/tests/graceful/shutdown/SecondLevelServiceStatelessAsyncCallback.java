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

package joynr.tests.graceful.shutdown;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.ReplyContext;
import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.UsedBy;


@UsedBy(SecondLevelServiceProxy.class)
public interface SecondLevelServiceStatelessAsyncCallback extends StatelessAsyncCallback {


	/*
	* transform
	*/
	@StatelessCallbackCorrelation("-2140771398")
	default void transformSuccess(
			String transformedData,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("transformSuccess not implemented for callback instance"); }
	default void transformFailed(
			JoynrRuntimeException runtimeException,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("transformFailed with exception not implemented for callback instance"); }
}
