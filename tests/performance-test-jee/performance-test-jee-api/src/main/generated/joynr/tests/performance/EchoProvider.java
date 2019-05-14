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

import io.joynr.provider.Promise;
import io.joynr.provider.Deferred;
import io.joynr.provider.AbstractDeferred;
import io.joynr.provider.DeferredVoid;

import io.joynr.provider.JoynrInterface;
import io.joynr.JoynrVersion;

import joynr.tests.performance.Types.ComplexStruct;

import io.joynr.provider.SubscriptionPublisherInjection;

interface EchoSubscriptionPublisherInjection extends SubscriptionPublisherInjection<EchoSubscriptionPublisher> {}

@JoynrInterface(provides = Echo.class, provider = EchoProvider.class, name = "tests/performance/Echo")
@JoynrVersion(major = 0, minor = 1)
public interface EchoProvider extends EchoSubscriptionPublisherInjection {


	Promise<Deferred<String>> getSimpleAttribute();
	Promise<DeferredVoid> setSimpleAttribute(String simpleAttribute);

	/**
	 * echoString
	 * @param data the parameter data
	 * @return promise for asynchronous handling
	 */
	public Promise<EchoStringDeferred> echoString(
			String data
	);

	/**
	 * echoByteArray
	 * @param data the parameter data
	 * @return promise for asynchronous handling
	 */
	public Promise<EchoByteArrayDeferred> echoByteArray(
			Byte[] data
	);

	/**
	 * echoComplexStruct
	 * @param data the parameter data
	 * @return promise for asynchronous handling
	 */
	public Promise<EchoComplexStructDeferred> echoComplexStruct(
			ComplexStruct data
	);

	public class EchoStringDeferred extends AbstractDeferred {
		public synchronized boolean resolve(String responseData) {
			return super.resolve(responseData);
		}
	}

	public class EchoByteArrayDeferred extends AbstractDeferred {
		public synchronized boolean resolve(Byte[] responseData) {
			return super.resolve((Object)responseData);
		}
	}

	public class EchoComplexStructDeferred extends AbstractDeferred {
		public synchronized boolean resolve(ComplexStruct responseData) {
			return super.resolve(responseData);
		}
	}
}
