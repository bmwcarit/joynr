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
package joynr.test;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.provider.Promise;


public class DefaultSitControllerProvider extends SitControllerAbstractProvider {
	private static final Logger logger = LoggerFactory.getLogger(DefaultSitControllerProvider.class);


	public DefaultSitControllerProvider() {
	}



	/*
	* ping
	*/
	@Override
	public Promise<PingDeferred> ping(
			) {
		logger.warn("**********************************************");
		logger.warn("* DefaultSitControllerProvider.ping called");
		logger.warn("**********************************************");
		PingDeferred deferred = new PingDeferred();
		String pingOut = "";
		deferred.resolve(pingOut);
		return new Promise<>(deferred);
	}

	/*
	* triggerTests
	*/
	@Override
	public Promise<TriggerTestsDeferred> triggerTests(
			) {
		logger.warn("**********************************************");
		logger.warn("* DefaultSitControllerProvider.triggerTests called");
		logger.warn("**********************************************");
		TriggerTestsDeferred deferred = new TriggerTestsDeferred();
		String output = "";
		deferred.resolve(output);
		return new Promise<>(deferred);
	}

	/*
	* waitForStatelessResult
	*/
	@Override
	public Promise<WaitForStatelessResultDeferred> waitForStatelessResult(
			Integer timeoutMs) {
		logger.warn("**********************************************");
		logger.warn("* DefaultSitControllerProvider.waitForStatelessResult called");
		logger.warn("**********************************************");
		WaitForStatelessResultDeferred deferred = new WaitForStatelessResultDeferred();
		String result = "";
		deferred.resolve(result);
		return new Promise<>(deferred);
	}
}
