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

import io.joynr.StatelessAsync;
import io.joynr.dispatcher.rpc.annotation.StatelessCallbackCorrelation;
import io.joynr.proxy.MessageIdCallback;
import io.joynr.UsedBy;


@StatelessAsync
@UsedBy(SitControllerProxy.class)
public interface SitControllerStatelessAsync extends SitController {


	/*
	* ping
	*/
	@StatelessCallbackCorrelation("131989339")
	void ping(
			MessageIdCallback messageIdCallback
	);

	/*
	* triggerTests
	*/
	@StatelessCallbackCorrelation("-1414043045")
	void triggerTests(
			MessageIdCallback messageIdCallback
	);

	/*
	* waitForStatelessResult
	*/
	@StatelessCallbackCorrelation("-2021832991")
	void waitForStatelessResult(
			Integer timeoutMs,
			MessageIdCallback messageIdCallback
	);
}
