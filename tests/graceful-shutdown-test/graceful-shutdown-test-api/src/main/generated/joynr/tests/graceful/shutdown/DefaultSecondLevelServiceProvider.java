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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.provider.Promise;
import io.joynr.provider.DeferredVoid;


public class DefaultSecondLevelServiceProvider extends SecondLevelServiceAbstractProvider {
	private static final Logger logger = LoggerFactory.getLogger(DefaultSecondLevelServiceProvider.class);


	public DefaultSecondLevelServiceProvider() {
	}



	/*
	* transform
	*/
	@Override
	public Promise<TransformDeferred> transform(
			String inData) {
		logger.warn("**********************************************");
		logger.warn("* DefaultSecondLevelServiceProvider.transform called");
		logger.warn("**********************************************");
		TransformDeferred deferred = new TransformDeferred();
		String transformedData = "";
		deferred.resolve(transformedData);
		return new Promise<>(deferred);
	}

	/*
	* logFailure
	*/
	@Override
	public void logFailure(
			String inData) {
		logger.warn("**********************************************");
		logger.warn("* DefaultSecondLevelServiceProvider.logFailure called");
		logger.warn("**********************************************");
	}
}
