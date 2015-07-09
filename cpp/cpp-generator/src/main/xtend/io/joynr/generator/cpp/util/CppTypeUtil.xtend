package io.joynr.generator.cpp.util
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

import com.google.inject.Inject
import io.joynr.generator.util.TypeUtil
import org.franca.core.franca.FArgument
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType

abstract class CppTypeUtil extends TypeUtil {
	@Inject
	private extension JoynrCppGeneratorExtensions

	def getCommaSeperatedUntypedInputParameterList(FMethod method) {
		getCommaSeperatedUntypedParameterList(method.inputParameters);
	}

	def getCommaSeperatedUntypedOutputParameterList(FMethod method) {
		getCommaSeperatedUntypedParameterList(method.outputParameters);
	}

	def getCommaSeperatedUntypedParameterList(Iterable<FArgument> arguments) {
		val returnStringBuilder = new StringBuilder();
		for (argument : arguments) {
			returnStringBuilder.append(argument.joynrName)
			returnStringBuilder.append(", ")
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		}
		return returnString.substring(0, returnString.length() - 2); //remove the last ,
	}

	def String getCommaSeparatedOutputParameterTypes(FMethod method) {
		getCommaSeparatedParameterTypes(method.outputParameters)
	}

	def String getCommaSeparatedOutputParameterTypes(FBroadcast broadcast) {
		getCommaSeparatedParameterTypes(broadcast.outputParameters)
	}

	private def String getCommaSeparatedParameterTypes(Iterable<FArgument> arguments) {
		val commaSeparatedParams = new StringBuilder();
		for (parameter : arguments.mapParametersToTypeName) {
			commaSeparatedParams.append(parameter);
			commaSeparatedParams.append(", ");
		}
		val returnString = commaSeparatedParams.toString();
		if (returnString.length() == 0) {
			return "";
		} else {
			return returnString.substring(0, returnString.length() - 2); //remove the last ,
		}
	}

	private def getCommaSeperatedTypedParameterList(
		Iterable<FArgument> arguments,
		boolean constParameters,
		boolean parameterAsReference
	) {
		val returnStringBuilder = new StringBuilder();
		for(FArgument argument : arguments){
			returnStringBuilder.append("\n")
			if (constParameters) {
				returnStringBuilder.append("const ");
			}

			returnStringBuilder.append(argument.typeName);

			if (parameterAsReference) {
				returnStringBuilder.append("&");
			}

			returnStringBuilder.append(" ");
			returnStringBuilder.append(argument.joynrName);
			returnStringBuilder.append(",");
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		} else {
			return returnString.substring(0, returnString.length() - 1); //remove the last " ," or "\n,"
		}
	}

	def getCommaSeperatedTypedOutputParameterList(FMethod method) {
		getCommaSeperatedTypedParameterList(method.outputParameters, false, true)
	}

	def getCommaSeperatedTypedConstOutputParameterList(FMethod method) {
		getCommaSeperatedTypedParameterList(method.outputParameters, true, true)
	}

	def getCommaSeperatedTypedOutputParameterList(FBroadcast broadcast) {
		getCommaSeperatedTypedParameterList(broadcast.outputParameters, false, true)
	}

	def getCommaSeperatedTypedConstOutputParameterList(FBroadcast broadcast) {
		getCommaSeperatedTypedParameterList(broadcast.outputParameters, true, true)
	}

	def getCommaSeperatedTypedConstInputParameterList(FMethod method) {
		getCommaSeperatedTypedParameterList(method.inputParameters, true, true)
	}

	override getTypeName(FType datatype) {
		val packagepath = buildPackagePath(datatype, "::");
		if (isEnum(datatype)){
			return  packagepath + datatype.joynrName+ "::" + getNestedEnumName();
		}
		else{
			return  packagepath + datatype.joynrName  //if we don't know the type, we have to assume its a complex datatype defined somewhere else.
		}
	}

	override getTypeNameForList(FType datatype) {
		"QList<" + datatype.typeName + "> ";
	}

	override getTypeNameForList(FBasicTypeId datatype) {
		"QList<" + datatype.typeName + "> ";
	}

}