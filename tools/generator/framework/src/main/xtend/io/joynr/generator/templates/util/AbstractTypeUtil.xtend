package io.joynr.generator.templates.util
/*
 * !!!
 *
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
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

	def String getTypeName(FType datatype)

	def String getTypeNameForList(FBasicTypeId datatype)

	def String getTypeNameForList(FType datatype)

	def String getTypeName(FTypeRef type) {
		if (type.derived != null) {
			type.derived.typeName
		} else {
			type.predefined.typeName
		}
	}

	def String getTypeNameForList(FTypeRef type) {
		if (type.derived != null) {
			type.derived.typeNameForList
		} else {
			type.predefined.typeNameForList
		}
	}

	def String getTypeName (FTypedElement typedElement) {
		var result =
				if (typedElement.array)
					typedElement.type.typeNameForList
				else
					typedElement.type.typeName
		if (result == null) {
			throw new IllegalStateException ("Datatype for element " + typedElement.name + " could not be found");
		}
		return result;
	}

	def Iterable<String> mapParametersToTypeName(FMethod method) {
		mapParametersToTypeName(method.outArgs)
	}

	// Convert an collection of parameters to their typenames
	def Iterable<String> mapParametersToTypeName(Iterable<FArgument> parameters) {
		val result = new ArrayList<String>();
		if (parameters.empty) {
			result.add("void");
		} else {
			for (FArgument parameter : parameters) {
				result.add(parameter.typeName)
			}
		}
		return result;
	}

	def getTypeNamesForOutputParameter (FMethod method) {
		val result = new ArrayList<String>();
		val types = method.outArgs;
		if (types == null || types.empty) {
			result.add("void");
		}
		for (FArgument argument : types) {
			result.add(argument.typeName);
		}
		return result;
	}

	/**
	 * @param method the method for which the signature shall be created
	 * @return a method signature that is unique in terms of method name, in
	 *      parameter names and in parameter types.
	 */
	def createMethodSignatureFromInParameters(FMethod method) {
		createParameterSignatureForMethod(method.name, method.inArgs.filterNull);
	}

	/**
	 * @param method the method for which the signature shall be created
	 * @return a method signature that is unique in terms of method name, out
	 *      parameter names and out parameter types.
	 */
	def createMethodSignatureFromOutParameters(FMethod method) {
		createParameterSignatureForMethod(method.name, method.outArgs.filterNull);
	}

	private def createParameterSignatureForMethod(String methodName, Iterable<FArgument> arguments) {
		val nameStringBuilder = new StringBuilder(methodName);
		for (FArgument argument : arguments) {
			nameStringBuilder.append(argument.name.toFirstUpper);
			val typeName = new StringBuilder(argument.typeName.objectDataTypeForPlainType);
			if (typeName.toString().contains("List")) {
				typeName.deleteCharAt(4);
				typeName.deleteCharAt(typeName.length-1);
			}
			nameStringBuilder.append(typeName.toString());
		}
		return nameStringBuilder.toString;
	}
}