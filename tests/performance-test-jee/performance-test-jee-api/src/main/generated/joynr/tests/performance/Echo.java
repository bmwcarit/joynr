
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

import java.util.List;
import java.util.Set;
import java.util.HashSet;
import io.joynr.subtypes.JoynrType;
import io.joynr.ProvidesJoynrTypesInfo;

import joynr.tests.performance.Types.ComplexStruct;

//The current generator is not able to check wether some of the imports are acutally necessary for this specific interface.
//Therefore some imports migth be unused in this version of the interface.
//To prevent warnings @SuppressWarnings("unused") is being used.
//To prevent warnings about an unnecessary SuppressWarnings we have to import something that is not used. (e.g. TreeSet)
import java.util.TreeSet;
@ProvidesJoynrTypesInfo(interfaceClass = Echo.class, interfaceName = "tests/performance/Echo")
@SuppressWarnings("unused")
public interface Echo {
	public static String INTERFACE_NAME = "tests/performance/Echo";


	public static Set<Class<?>> getDataTypes() {
		Set<Class<?>> set = new HashSet<>();
		if (JoynrType.class.isAssignableFrom(joynr.tests.performance.Types.ComplexStruct.class)) {
			set.add(joynr.tests.performance.Types.ComplexStruct.class);
		}
		return set;
	}

}

