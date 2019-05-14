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

import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.Async;
import io.joynr.ProvidedBy;
import io.joynr.UsedBy;



@Async
@ProvidedBy(EchoProvider.class)
@UsedBy(EchoProxy.class)
public interface EchoAsync extends Echo {





	/*
	* echoString
	*/
	public Future<String> echoString(
			@JoynrRpcCallback(deserializationType = String.class) Callback<String> callback,
			String data
	);
	default public Future<String> echoString(
			@JoynrRpcCallback(deserializationType = String.class) Callback<String> callback,
			String data,
			MessagingQos messagingQos
	) {
		return echoString(
			callback,
			data
		);
	}

}
