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

import com.google.inject.Inject
import com.google.inject.Singleton
import java.util.HashMap
import java.util.HashSet
import org.franca.core.franca.FArgument
import org.franca.core.franca.FMethod
import org.franca.core.franca.FEnumerationType

@Singleton
public class MethodUtil {

	@Inject
	private extension TypeUtil;

	@Inject
	private extension NamingUtil;

	def Iterable<FArgument> getOutputParameters(FMethod method) {
		if (method == null || method.outArgs.size() == 0){
			return new HashSet<FArgument>
		}
		else{
			return method.outArgs.filterNull
		}
	}


	def Iterable<FArgument> getInputParameters(FMethod method) {
		if (method == null || method.inArgs.size() == 0){
			return new HashSet<FArgument>
		}
		else{
			return method.inArgs.filterNull
		}
	}


	def getAllRequiredTypes(FMethod method, String methodErrorEnumName, boolean errorTypes) {
		var Object datatype = null
		var typeList = new HashSet<Object>();
		for(returnParameter : getOutputParameters(method).filterNull){
			typeList.addAll(getRequiredTypes(returnParameter.type));
		}
		for (inputParameter : getInputParameters(method).filterNull) {
			typeList.addAll(getRequiredTypes(inputParameter.type));
		}
		if (errorTypes) {
			if (method.errors !== null) {
				datatype = getDatatype(method.errors);
				method.errors.name = methodErrorEnumName
				if (datatype != null) {
					typeList.add(datatype)
				}
			}
			if (method.errorEnum !== null) {
				datatype = getDatatype(method.errorEnum);
				if (datatype != null) {
					typeList.add(datatype)
				}
			}
		}

		return typeList
	}

	/**
	 * @return a mapping from method names to the number of their overloads.
	 */
	def overloadedMethodCounts(Iterable<FMethod> methods) {
		var methodNameToCount = new HashMap<String, Integer>();

		for (FMethod method : methods) {
			var Integer count = 1;
			if (methodNameToCount.containsKey(method.name)) {
				count = methodNameToCount.get(method.name);
				count++;
			}
			methodNameToCount.put(method.name, count);
		}
		return methodNameToCount;
	}

	def boolean hasErrorEnum(FMethod method) {
		return (method.errors != null) || (method.errorEnum != null);
	}

	def boolean hasEnumInputParameter(FMethod method) {
		for (inputParameter : getInputParameters(method).filterNull) {
			if (getDatatype(inputParameter.type) instanceof FEnumerationType) {
				return true;
			};
		}
		return false
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
			var typeName = argument.type.joynrName;
			if (isArray(argument)) {
				typeName = "List"+typeName
			}
			nameStringBuilder.append(typeName.objectDataTypeForPlainType);
		}
		return nameStringBuilder.toString;
	}

}