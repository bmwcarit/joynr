
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

import java.util.List;
import java.util.Set;
import java.util.HashSet;
import io.joynr.subtypes.JoynrType;
import io.joynr.ProvidesJoynrTypesInfo;


//The current generator is not able to check wether some of the imports are acutally necessary for this specific interface.
//Therefore some imports migth be unused in this version of the interface.
//To prevent warnings @SuppressWarnings("unused") is being used.
//To prevent warnings about an unnecessary SuppressWarnings we have to import something that is not used. (e.g. TreeSet)
import java.util.TreeSet;
@ProvidesJoynrTypesInfo(interfaceClass = SystemIntegrationTest.class, interfaceName = "test/SystemIntegrationTest")
@SuppressWarnings("unused")
public interface SystemIntegrationTest {
	public static String INTERFACE_NAME = "test/SystemIntegrationTest";


	public static Set<Class<?>> getDataTypes() {
		Set<Class<?>> set = new HashSet<>();
		return set;
	}

}

