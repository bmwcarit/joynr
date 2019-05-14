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


public class DefaultEchoProvider extends EchoAbstractProvider {
	private static final Logger logger = LoggerFactory.getLogger(DefaultEchoProvider.class);


	public DefaultEchoProvider() {
	}



	/*
	* echoString
	*/
	@Override
	public Promise<EchoStringDeferred> echoString(
			String data) {
		logger.warn("**********************************************");
		logger.warn("* DefaultEchoProvider.echoString called");
		logger.warn("**********************************************");
		EchoStringDeferred deferred = new EchoStringDeferred();
		String responseData = "";
		deferred.resolve(responseData);
		return new Promise<>(deferred);
	}
}
