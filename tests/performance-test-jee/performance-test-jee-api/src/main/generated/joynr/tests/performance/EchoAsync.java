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

import io.joynr.messaging.MessagingQos;
import io.joynr.proxy.Callback;
import io.joynr.proxy.Future;
import io.joynr.dispatcher.rpc.annotation.JoynrRpcCallback;
import io.joynr.Async;
import io.joynr.ProvidedBy;
import io.joynr.UsedBy;
import io.joynr.exceptions.DiscoveryException;

import joynr.tests.performance.Types.ComplexStruct;


@Async
@ProvidedBy(EchoProvider.class)
@UsedBy(EchoProxy.class)
public interface EchoAsync extends Echo {

	public Future<String> getSimpleAttribute(@JoynrRpcCallback(deserializationType = String.class) Callback<String> callback);
	default public Future<String> getSimpleAttribute(@JoynrRpcCallback(deserializationType = String.class) Callback<String> callback, MessagingQos messagingQos) {
		return getSimpleAttribute(callback);
	}
	Future<Void> setSimpleAttribute(@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback, String simpleAttribute) throws DiscoveryException;
	default Future<Void> setSimpleAttribute(@JoynrRpcCallback(deserializationType = Void.class) Callback<Void> callback, String simpleAttribute, MessagingQos messagingQos) throws DiscoveryException {
		return setSimpleAttribute(callback, simpleAttribute);
	}




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


	/*
	* echoByteArray
	*/
	public Future<Byte[]> echoByteArray(
			@JoynrRpcCallback(deserializationType = Byte[].class) Callback<Byte[]> callback,
			Byte[] data
	);
	default public Future<Byte[]> echoByteArray(
			@JoynrRpcCallback(deserializationType = Byte[].class) Callback<Byte[]> callback,
			Byte[] data,
			MessagingQos messagingQos
	) {
		return echoByteArray(
			callback,
			data
		);
	}


	/*
	* echoComplexStruct
	*/
	public Future<ComplexStruct> echoComplexStruct(
			@JoynrRpcCallback(deserializationType = ComplexStruct.class) Callback<ComplexStruct> callback,
			ComplexStruct data
	);
	default public Future<ComplexStruct> echoComplexStruct(
			@JoynrRpcCallback(deserializationType = ComplexStruct.class) Callback<ComplexStruct> callback,
			ComplexStruct data,
			MessagingQos messagingQos
	) {
		return echoComplexStruct(
			callback,
			data
		);
	}

}
