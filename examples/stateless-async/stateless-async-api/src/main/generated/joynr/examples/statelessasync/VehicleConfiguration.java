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
import java.io.Serializable;

import io.joynr.subtypes.JoynrType;

import joynr.examples.statelessasync.KeyValue;
import edu.umd.cs.findbugs.annotations.SuppressFBWarnings;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonIgnore;

// NOTE: serialVersionUID is not defined since we don't support Franca versions right now.
//       The compiler will generate a serialVersionUID based on the class and its members
//       (cf. http://docs.oracle.com/javase/6/docs/platform/serialization/spec/class.html#4100),
//       which is probably more restrictive than what we want.

/**
 */
@SuppressWarnings("serial")
public class VehicleConfiguration implements Serializable, JoynrType {
	public static final int MAJOR_VERSION = 0;
	public static final int MINOR_VERSION = 1;
	@JsonProperty("id")
	private String id;
	@JsonProperty("entries")
	private KeyValue[] entries = {};

	/**
	 * Default Constructor
	 */
	public VehicleConfiguration() {
		this.id = "";
	}

	/**
	 * Copy constructor
	 *
	 * @param vehicleConfigurationObj reference to the object to be copied
	 */
	public VehicleConfiguration(VehicleConfiguration vehicleConfigurationObj) {
		this.id = vehicleConfigurationObj.id;
		this.entries = vehicleConfigurationObj.entries;
	}

	/**
	 * Parameterized constructor
	 *
	 * @param id description missing in Franca model.
	 * @param entries description missing in Franca model.
	 */
	@SuppressFBWarnings(value = "EI_EXPOSE_REP2", justification = "joynr object not used for storing internal state")
	public VehicleConfiguration(
		String id,
		KeyValue[] entries
		) {
		this.id = id;
		this.entries = entries;
	}

	/**
	 * Gets Id
	 *
	 * @return 
	 */
	@JsonIgnore
	public String getId() {
		return this.id;
	}

	/**
	 * Sets Id
	 *
	 * @param id description missing in Franca model.
	 */
	@JsonIgnore
	public void setId(String id) {
		if (id == null) {
			throw new IllegalArgumentException("setting id to null is not allowed");
		}
		this.id = id;
	}

	/**
	 * Gets Entries
	 *
	 * @return 
	 */
	@SuppressFBWarnings(value = "EI_EXPOSE_REP", justification = "joynr object not used for storing internal state")
	@JsonIgnore
	public KeyValue[] getEntries() {
		return this.entries;
	}

	/**
	 * Sets Entries
	 *
	 * @param entries description missing in Franca model.
	 */
	@SuppressFBWarnings(value = "EI_EXPOSE_REP2", justification = "joynr object not used for storing internal state")
	@JsonIgnore
	public void setEntries(KeyValue[] entries) {
		if (entries == null) {
			throw new IllegalArgumentException("setting entries to null is not allowed");
		}
		this.entries = entries;
	}


	/**
	 * Stringifies the class
	 *
	 * @return stringified class content
	 */
	@Override
	public String toString() {
		return "VehicleConfiguration ["
		+ "id=" + this.id + ", "
		+ "entries=" + java.util.Arrays.toString(this.entries)
		+ "]";
	}

	/**
	 * Check for equality
	 *
	 * @param obj Reference to the object to compare to
	 * @return true, if objects are equal, false otherwise
	 */
	@Override
	public boolean equals(Object obj) {
		if (this == obj)
			return true;
		if (obj == null)
			return false;
		if (getClass() != obj.getClass())
			return false;
		VehicleConfiguration other = (VehicleConfiguration) obj;
		if (this.id == null) {
			if (other.id != null) {
				return false;
			}
		} else if (!this.id.equals(other.id)){
			return false;
		}
		if (this.entries == null) {
			if (other.entries != null) {
				return false;
			}
		} else if (!java.util.Arrays.deepEquals(this.entries, other.entries)){
			return false;
		}
		return true;
	}

	/**
	 * Calculate code for hashing based on member contents
	 *
	 * @return The calculated hash code
	 */
	@Override
	public int hashCode() {
		int result = 1;
		final int prime = 31;
		result = prime * result + ((this.id == null) ? 0 : this.id.hashCode());
		result = prime * result + ((this.entries == null) ? 0 : java.util.Arrays.hashCode(this.entries));
		return result;
	}
}

	
