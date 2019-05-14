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

package joynr.tests.performance.Types;
import java.io.Serializable;

import io.joynr.subtypes.JoynrType;

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
public class ComplexStruct implements Serializable, JoynrType {
	public static final int MAJOR_VERSION = 0;
	public static final int MINOR_VERSION = 1;
	@JsonProperty("num32")
	private Integer num32;
	@JsonProperty("num64")
	private Long num64;
	@JsonProperty("data")
	private Byte[] data = {};
	@JsonProperty("str")
	private String str;

	/**
	 * Default Constructor
	 */
	public ComplexStruct() {
		this.num32 = 0;
		this.num64 = 0L;
		this.str = "";
	}

	/**
	 * Copy constructor
	 *
	 * @param complexStructObj reference to the object to be copied
	 */
	public ComplexStruct(ComplexStruct complexStructObj) {
		this.num32 = complexStructObj.num32;
		this.num64 = complexStructObj.num64;
		this.data = complexStructObj.data;
		this.str = complexStructObj.str;
	}

	/**
	 * Parameterized constructor
	 *
	 * @param num32 description missing in Franca model.
	 * @param num64 description missing in Franca model.
	 * @param data description missing in Franca model.
	 * @param str description missing in Franca model.
	 */
	@SuppressFBWarnings(value = "EI_EXPOSE_REP2", justification = "joynr object not used for storing internal state")
	public ComplexStruct(
		Integer num32,
		Long num64,
		Byte[] data,
		String str
		) {
		this.num32 = num32;
		this.num64 = num64;
		this.data = data;
		this.str = str;
	}

	/**
	 * Gets Num32
	 *
	 * @return 
	 */
	@JsonIgnore
	public Integer getNum32() {
		return this.num32;
	}

	/**
	 * Sets Num32
	 *
	 * @param num32 description missing in Franca model.
	 */
	@JsonIgnore
	public void setNum32(Integer num32) {
		if (num32 == null) {
			throw new IllegalArgumentException("setting num32 to null is not allowed");
		}
		this.num32 = num32;
	}

	/**
	 * Gets Num64
	 *
	 * @return 
	 */
	@JsonIgnore
	public Long getNum64() {
		return this.num64;
	}

	/**
	 * Sets Num64
	 *
	 * @param num64 description missing in Franca model.
	 */
	@JsonIgnore
	public void setNum64(Long num64) {
		if (num64 == null) {
			throw new IllegalArgumentException("setting num64 to null is not allowed");
		}
		this.num64 = num64;
	}

	/**
	 * Gets Data
	 *
	 * @return 
	 */
	@SuppressFBWarnings(value = "EI_EXPOSE_REP", justification = "joynr object not used for storing internal state")
	@JsonIgnore
	public Byte[] getData() {
		return this.data;
	}

	/**
	 * Sets Data
	 *
	 * @param data description missing in Franca model.
	 */
	@SuppressFBWarnings(value = "EI_EXPOSE_REP2", justification = "joynr object not used for storing internal state")
	@JsonIgnore
	public void setData(Byte[] data) {
		if (data == null) {
			throw new IllegalArgumentException("setting data to null is not allowed");
		}
		this.data = data;
	}

	/**
	 * Gets Str
	 *
	 * @return 
	 */
	@JsonIgnore
	public String getStr() {
		return this.str;
	}

	/**
	 * Sets Str
	 *
	 * @param str description missing in Franca model.
	 */
	@JsonIgnore
	public void setStr(String str) {
		if (str == null) {
			throw new IllegalArgumentException("setting str to null is not allowed");
		}
		this.str = str;
	}


	/**
	 * Stringifies the class
	 *
	 * @return stringified class content
	 */
	@Override
	public String toString() {
		return "ComplexStruct ["
		+ "num32=" + this.num32 + ", "
		+ "num64=" + this.num64 + ", "
		+ "data=" + java.util.Arrays.toString(this.data) + ", "
		+ "str=" + this.str
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
		ComplexStruct other = (ComplexStruct) obj;
		if (this.num32 == null) {
			if (other.num32 != null) {
				return false;
			}
		} else if (!this.num32.equals(other.num32)){
			return false;
		}
		if (this.num64 == null) {
			if (other.num64 != null) {
				return false;
			}
		} else if (!this.num64.equals(other.num64)){
			return false;
		}
		if (this.data == null) {
			if (other.data != null) {
				return false;
			}
		} else if (!java.util.Arrays.deepEquals(this.data, other.data)){
			return false;
		}
		if (this.str == null) {
			if (other.str != null) {
				return false;
			}
		} else if (!this.str.equals(other.str)){
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
		result = prime * result + ((this.num32 == null) ? 0 : this.num32.hashCode());
		result = prime * result + ((this.num64 == null) ? 0 : this.num64.hashCode());
		result = prime * result + ((this.data == null) ? 0 : java.util.Arrays.hashCode(this.data));
		result = prime * result + ((this.str == null) ? 0 : this.str.hashCode());
		return result;
	}
}

	
