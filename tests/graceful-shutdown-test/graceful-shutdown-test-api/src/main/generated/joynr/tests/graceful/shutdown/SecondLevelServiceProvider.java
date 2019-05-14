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

import io.joynr.provider.Promise;
import io.joynr.provider.AbstractDeferred;
import io.joynr.provider.DeferredVoid;

import io.joynr.provider.JoynrInterface;
import io.joynr.JoynrVersion;


@JoynrInterface(provides = SecondLevelService.class, provider = SecondLevelServiceProvider.class, name = "tests/graceful/shutdown/SecondLevelService")
@JoynrVersion(major = 0, minor = 1)
public interface SecondLevelServiceProvider {


	/**
	 * transform
	 * @param inData the parameter inData
	 * @return promise for asynchronous handling
	 */
	public Promise<TransformDeferred> transform(
			String inData
	);

	/**
	 * logFailure
	 * @param inData the parameter inData
	 * @return promise for asynchronous handling
	 */
	public void logFailure(
			String inData
	);

	public class TransformDeferred extends AbstractDeferred {
		public synchronized boolean resolve(String transformedData) {
			return super.resolve(transformedData);
		}
	}
}
