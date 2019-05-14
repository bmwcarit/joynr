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
import io.joynr.Sync;
import io.joynr.ProvidedBy;
import io.joynr.UsedBy;

import joynr.tests.performance.Types.ComplexStruct;

@Sync
@ProvidedBy(EchoProvider.class)
@UsedBy(EchoProxy.class)
public interface EchoSync extends Echo {

	public String getSimpleAttribute();
	default public String getSimpleAttribute(MessagingQos messagingQos) {
		return getSimpleAttribute();
	}
	void setSimpleAttribute(String simpleAttribute);
	default void setSimpleAttribute(String simpleAttribute, MessagingQos messagingQos) {
	}


	/*
	* echoString
	*/
	public String echoString(
			String data
	);
	default public String echoString(
			String data,
			MessagingQos messagingQos
	) {
		return echoString(
			data
		);
	}

	/*
	* echoByteArray
	*/
	public Byte[] echoByteArray(
			Byte[] data
	);
	default public Byte[] echoByteArray(
			Byte[] data,
			MessagingQos messagingQos
	) {
		return echoByteArray(
			data
		);
	}

	/*
	* echoComplexStruct
	*/
	public ComplexStruct echoComplexStruct(
			ComplexStruct data
	);
	default public ComplexStruct echoComplexStruct(
			ComplexStruct data,
			MessagingQos messagingQos
	) {
		return echoComplexStruct(
			data
		);
	}
}
