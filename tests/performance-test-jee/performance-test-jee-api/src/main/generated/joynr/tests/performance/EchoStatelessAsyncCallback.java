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

package joynr.tests.performance;

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.proxy.ReplyContext;
import io.joynr.proxy.StatelessAsyncCallback;
import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.UsedBy;

import joynr.tests.performance.Types.ComplexStruct;

@UsedBy(EchoProxy.class)
public interface EchoStatelessAsyncCallback extends StatelessAsyncCallback {

	/*
	* simpleAttribute getter
	*/
	@StatelessCallbackCorrelation("2065380052")
	default void getSimpleAttributeSuccess(String simpleAttribute, ReplyContext replyContext)
	{ throw new UnsupportedOperationException("getSimpleAttributeSuccess not implemented for callback instance"); }
	@StatelessCallbackCorrelation("2065380052")
	default void getSimpleAttributeFailed(
			JoynrRuntimeException runtimeException,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("getSimpleAttributeFailed not implemented for callback instance"); }
	/*
	* simpleAttribute setter
	*/
	@StatelessCallbackCorrelation("-1309553592")
	default void setSimpleAttributeSuccess(ReplyContext replyContext)
	{ throw new UnsupportedOperationException("setSimpleAttributeSuccess not implemented for callback instance"); }
	@StatelessCallbackCorrelation("-1309553592")
	default void setSimpleAttributeFailed(
			JoynrRuntimeException runtimeException,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("setSimpleAttributeFailed not implemented for callback instance"); }

	/*
	* echoString
	*/
	@StatelessCallbackCorrelation("-1172639566")
	default void echoStringSuccess(
			String responseData,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("echoStringSuccess not implemented for callback instance"); }
	default void echoStringFailed(
			JoynrRuntimeException runtimeException,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("echoStringFailed with exception not implemented for callback instance"); }

	/*
	* echoByteArray
	*/
	@StatelessCallbackCorrelation("-1728974786")
	default void echoByteArraySuccess(
			Byte[] responseData,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("echoByteArraySuccess not implemented for callback instance"); }
	default void echoByteArrayFailed(
			JoynrRuntimeException runtimeException,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("echoByteArrayFailed with exception not implemented for callback instance"); }

	/*
	* echoComplexStruct
	*/
	@StatelessCallbackCorrelation("716873786")
	default void echoComplexStructSuccess(
			ComplexStruct responseData,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("echoComplexStructSuccess not implemented for callback instance"); }
	default void echoComplexStructFailed(
			JoynrRuntimeException runtimeException,
			ReplyContext replyContext
	) { throw new UnsupportedOperationException("echoComplexStructFailed with exception not implemented for callback instance"); }
}
