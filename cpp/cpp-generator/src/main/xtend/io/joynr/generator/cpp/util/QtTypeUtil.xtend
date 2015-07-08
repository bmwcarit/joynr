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

import org.franca.core.franca.FArgument
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FMethod
import org.franca.core.franca.FTypedElement

class QtTypeUtil extends CppTypeUtil {

	override getTypeName(FBasicTypeId datatype) {
		switch datatype {
			case FBasicTypeId::BOOLEAN: "bool"
			case FBasicTypeId::INT8: "qint8"
			case FBasicTypeId::UINT8: "qint8"
			case FBasicTypeId::INT16: "int"
			case FBasicTypeId::UINT16: "int"
			case FBasicTypeId::INT32: "int"
			case FBasicTypeId::UINT32: "int"
			case FBasicTypeId::INT64: "qint64"
			case FBasicTypeId::UINT64: "qint64"
			case FBasicTypeId::FLOAT: "double"
			case FBasicTypeId::DOUBLE: "double"
			case FBasicTypeId::STRING: "QString"
			case FBasicTypeId::BYTE_BUFFER: "QByteArray"
			default: throw new IllegalArgumentException("Unsupported basic type: " + datatype.getName)
		}
	}

	def fromStdTypeToQTType(FTypedElement typedElement, String objectName) {
		if (typedElement.type.predefined != null) {
			if (typedElement.type.predefined === FBasicTypeId.STRING) {
				if (typedElement.isArray) {
					return '''TypeUtil::convertStdStringstoQStrings(«objectName»)'''
				}
				else {
					return '''TypeUtil::convertStdStringtoQString(«objectName»)'''
				}
			}
		}
		//by default, return objectName
		return objectName
	}

	def fromQTTypeToStdType(FTypedElement typedElement, String objectName) {
		if (typedElement.type.predefined != null) {
			if (typedElement.type.predefined === FBasicTypeId.STRING) {
				if (typedElement.isArray) {
					return '''TypeUtil::convertQStringstoStdStrings(«objectName»)'''
				}
				else {
					return '''TypeUtil::convertQStringtoStdString(«objectName»)'''
				}
			}
		}
		//by default, return objectName
		return objectName;
 	}

	def getCommaSeperatedUntypedOutputParameterList(FMethod method, DatatypeSystemTransformation datatypeSystem) {
		getCommaSeperatedUntypedParameterList(method.outputParameters, datatypeSystem)
	}

	def getCommaSeperatedUntypedOutputParameterList(FBroadcast broadcast, DatatypeSystemTransformation datatypeSystem) {
		getCommaSeperatedUntypedParameterList(broadcast.outputParameters, datatypeSystem)
	}

	def getCommaSeperatedUntypedInputParameterList(FMethod method, DatatypeSystemTransformation datatypeSystem) {
		getCommaSeperatedUntypedParameterList(method.inputParameters, datatypeSystem)
	}

	def getCommaSeperatedUntypedParameterList(Iterable<FArgument> arguments, DatatypeSystemTransformation transformation) {
		val returnStringBuilder = new StringBuilder();
		for (argument : arguments) {
			returnStringBuilder.append(transformBetweenDatatypeSystems(argument, argument.joynrName, transformation))
			returnStringBuilder.append(", ")
		}
		val returnString = returnStringBuilder.toString();
		if (returnString.length() == 0) {
			return "";
		}
		return returnString.substring(0, returnString.length() - 2); //remove the last ,
	}

	private def transformBetweenDatatypeSystems(FArgument argument, String argumentName, DatatypeSystemTransformation transformation) {
		switch (transformation) {
			case FROM_QT_TO_STANDARD: return fromQTTypeToStdType(argument, argumentName)
			case FROM_STANDARD_TO_QT: return fromStdTypeToQTType(argument, argumentName)
			default : return argumentName
		}
	}

	def needsDatatypeConversion(FTypedElement typedElement) {
		typedElement.type.predefined === FBasicTypeId.STRING
	}

	def needsDatatypeConversion(FBroadcast broadcast) {
		for (outParam : broadcast.outputParameters) {
			if (outParam.type.predefined === FBasicTypeId.STRING) {
				return true;
			}
		}
		return false;
	}

 }
