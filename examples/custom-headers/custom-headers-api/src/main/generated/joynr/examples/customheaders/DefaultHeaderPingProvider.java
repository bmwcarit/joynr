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
package joynr.examples.customheaders;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.provider.Promise;


public class DefaultHeaderPingProvider extends HeaderPingAbstractProvider {
	private static final Logger logger = LoggerFactory.getLogger(DefaultHeaderPingProvider.class);


	public DefaultHeaderPingProvider() {
	}



	/*
	* ping
	*/
	@Override
	public Promise<PingDeferred> ping(
			) {
		logger.warn("**********************************************");
		logger.warn("* DefaultHeaderPingProvider.ping called");
		logger.warn("**********************************************");
		PingDeferred deferred = new PingDeferred();
		String headerReturn = "";
		deferred.resolve(headerReturn);
		return new Promise<>(deferred);
	}
}
