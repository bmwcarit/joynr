package io.joynr.generator.templates.util
/*
 * !!!
 *
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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

import java.util.ArrayList
import org.franca.core.franca.FArgument
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType
import org.franca.core.franca.FTypeRef
import org.franca.core.franca.FTypedElement

abstract class AbstractTypeUtil extends TypeUtil{

	def String getTypeName(FBasicTypeId datatype)

	def String getTypeName(FType datatype, boolean generateVersion)

	def String getTypeName(FType datatype)

	def String getTypeNameForList(FBasicTypeId datatype)

	def String getTypeNameForList(FType datatype)

	def String getTypeNameForList(FType datatype, boolean generateVersion)

	def String getTypeName(FTypeRef type, boolean generateVersion) {
		if (type.derived !== null) {
			type.derived.getTypeName(generateVersion)
		} else {
			type.predefined.typeName
		}
	}

	def String getTypeName(FTypeRef type) {
		if (type.derived !== null) {
			type.derived.typeName
		} else {
			type.predefined.typeName
		}
	}

	def String getTypeNameForList(FTypeRef type) {
		if (type.derived !== null) {
			type.derived.typeNameForList
		} else {
			type.predefined.typeNameForList
		}
	}

	def String getTypeNameForList(FTypeRef type, boolean generateVersion) {
		if (type.derived !== null) {
			type.derived.getTypeNameForList(generateVersion)
		} else {
			type.predefined.typeNameForList
		}
	}

	def String getTypeName(FTypedElement typedElement, boolean generateVersion) {
		var result =
				if (isArray(typedElement))
					typedElement.type.getTypeNameForList(generateVersion)
				else
					typedElement.type.getTypeName(generateVersion)
		if (result === null) {
			throw new IllegalStateException ("Datatype for element " + typedElement.name + " could not be found");
		}
		return result;
	}

	def String getTypeName(FTypedElement typedElement) {
		var result =
				if (isArray(typedElement))
					typedElement.type.typeNameForList
				else
					typedElement.type.getTypeName
		if (result === null) {
			throw new IllegalStateException ("Datatype for element " + typedElement.name + " could not be found");
		}
		return result;
	}

	def Iterable<String> mapParametersToTypeName(FMethod method, boolean generateVersion) {
		mapParametersToTypeName(method.outArgs, generateVersion)
	}

	// Convert an collection of parameters to their typenames
	def Iterable<String> mapParametersToTypeName(Iterable<FArgument> parameters, boolean generateVersion) {
		val result = new ArrayList<String>();
		if (parameters.empty) {
			result.add("void");
		} else {
			for (FArgument parameter : parameters) {
				result.add(parameter.getTypeName(generateVersion))
			}
		}
		return result;
	}

	def getTypeNamesForOutputParameter (FMethod method, boolean generateVersion) {
		val result = new ArrayList<String>();
		val types = method.outArgs;
		if (types === null || types.empty) {
			result.add("void");
		}
		for (FArgument argument : types) {
			result.add(argument.getTypeName(generateVersion));
		}
		return result;
	}

	def getTypeNamesForOutputParameter (FMethod method) {
		val result = new ArrayList<String>();
		val types = method.outArgs;
		if (types === null || types.empty) {
			result.add("void");
		}
		for (FArgument argument : types) {
			result.add(argument.typeName);
		}
		return result;
	}
}
