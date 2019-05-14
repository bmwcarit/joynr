
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
package joynr.examples.statelessasync;

import java.util.List;
import java.util.Set;
import java.util.HashSet;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import io.joynr.subtypes.JoynrType;
import io.joynr.ProvidesJoynrTypesInfo;

import joynr.examples.statelessasync.VehicleConfiguration;

//The current generator is not able to check wether some of the imports are acutally necessary for this specific interface.
//Therefore some imports migth be unused in this version of the interface.
//To prevent warnings @SuppressWarnings("unused") is being used.
//To prevent warnings about an unnecessary SuppressWarnings we have to import something that is not used. (e.g. TreeSet)
import java.util.TreeSet;
@ProvidesJoynrTypesInfo(interfaceClass = VehicleState.class, interfaceName = "examples/statelessasync/VehicleState")
@SuppressWarnings("unused")
public interface VehicleState {
	public static String INTERFACE_NAME = "examples/statelessasync/VehicleState";


	public static Set<Class<?>> getDataTypes() {
		Set<Class<?>> set = new HashSet<>();
		if (JoynrType.class.isAssignableFrom(joynr.examples.statelessasync.VehicleConfiguration.class)) {
			set.add(joynr.examples.statelessasync.VehicleConfiguration.class);
		}
		return set;
	}

	/**
	 */
	public enum GetCurrentConfigErrorEnum {
		/**
		 * 
		 */
		UNKNOWN_CONFIGURATION_ID
		;
	
		public static final int MAJOR_VERSION = 0;
		public static final int MINOR_VERSION = 0;
		static final Map<Integer, GetCurrentConfigErrorEnum> ordinalToEnumValues = new HashMap<>();
	
		static{
			ordinalToEnumValues.put(0, UNKNOWN_CONFIGURATION_ID);
		}
	
		/**
		 * Get the matching enum for an ordinal number
		 * @param ordinal The ordinal number
		 * @return The matching enum for the given ordinal number
		 */
		public static GetCurrentConfigErrorEnum getEnumValue(Integer ordinal) {
			return ordinalToEnumValues.get(ordinal);
		}
	
		/**
		 * Get the matching ordinal number for this enum
		 * @return The ordinal number representing this enum
		 */
		public Integer getOrdinal() {
			// TODO should we use a bidirectional map from a third-party library?
			Integer ordinal = null;
			for(Entry<Integer, GetCurrentConfigErrorEnum> entry : ordinalToEnumValues.entrySet()) {
				if(this == entry.getValue()) {
					ordinal = entry.getKey();
					break;
				}
			}
			return ordinal;
		}
	}

}

