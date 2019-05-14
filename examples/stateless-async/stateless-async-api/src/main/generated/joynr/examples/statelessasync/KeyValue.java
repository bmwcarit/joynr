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

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.annotation.JsonIgnore;

// NOTE: serialVersionUID is not defined since we don't support Franca versions right now.
//       The compiler will generate a serialVersionUID based on the class and its members
//       (cf. http://docs.oracle.com/javase/6/docs/platform/serialization/spec/class.html#4100),
//       which is probably more restrictive than what we want.

/**
 */
@SuppressWarnings("serial")
public class KeyValue implements Serializable, JoynrType {
	public static final int MAJOR_VERSION = 0;
	public static final int MINOR_VERSION = 1;
	@JsonProperty("key")
	private String key;
	@JsonProperty("value")
	private String value;

	/**
	 * Default Constructor
	 */
	public KeyValue() {
		this.key = "";
		this.value = "";
	}

	/**
	 * Copy constructor
	 *
	 * @param keyValueObj reference to the object to be copied
	 */
	public KeyValue(KeyValue keyValueObj) {
		this.key = keyValueObj.key;
		this.value = keyValueObj.value;
	}

	/**
	 * Parameterized constructor
	 *
	 * @param key description missing in Franca model.
	 * @param value description missing in Franca model.
	 */
	public KeyValue(
		String key,
		String value
		) {
		this.key = key;
		this.value = value;
	}

	/**
	 * Gets Key
	 *
	 * @return 
	 */
	@JsonIgnore
	public String getKey() {
		return this.key;
	}

	/**
	 * Sets Key
	 *
	 * @param key description missing in Franca model.
	 */
	@JsonIgnore
	public void setKey(String key) {
		if (key == null) {
			throw new IllegalArgumentException("setting key to null is not allowed");
		}
		this.key = key;
	}

	/**
	 * Gets Value
	 *
	 * @return 
	 */
	@JsonIgnore
	public String getValue() {
		return this.value;
	}

	/**
	 * Sets Value
	 *
	 * @param value description missing in Franca model.
	 */
	@JsonIgnore
	public void setValue(String value) {
		if (value == null) {
			throw new IllegalArgumentException("setting value to null is not allowed");
		}
		this.value = value;
	}


	/**
	 * Stringifies the class
	 *
	 * @return stringified class content
	 */
	@Override
	public String toString() {
		return "KeyValue ["
		+ "key=" + this.key + ", "
		+ "value=" + this.value
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
		KeyValue other = (KeyValue) obj;
		if (this.key == null) {
			if (other.key != null) {
				return false;
			}
		} else if (!this.key.equals(other.key)){
			return false;
		}
		if (this.value == null) {
			if (other.value != null) {
				return false;
			}
		} else if (!this.value.equals(other.value)){
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
		result = prime * result + ((this.key == null) ? 0 : this.key.hashCode());
		result = prime * result + ((this.value == null) ? 0 : this.value.hashCode());
		return result;
	}
}

	
