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
import java.util.HashSet
import org.franca.core.franca.FArgument
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FBroadcast
import org.franca.core.franca.FMethod
import org.franca.core.franca.FType
import org.franca.core.franca.FTypedElement

class QtTypeUtil extends CppTypeUtil {

	@Inject
	private CppStdTypeUtil stdTypeUtil

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
		fromStdTypeToQTType(typedElement, objectName, false)
	}

	def fromStdTypeToQTType(FTypedElement typedElement, String objectName, boolean convertComplexTypes) {
		if (typedElement.type.predefined != null && typedElement.type.predefined != FBasicTypeId.UNDEFINED) {
			if (typedElement.isArray) {
				return '''TypeUtil::toQt(«objectName»)'''
			}
			else {
				switch (typedElement.type.predefined) {
					case FBasicTypeId.BOOLEAN: return objectName
					case FBasicTypeId.INT8: return '''TypeUtil::toQt(«objectName»)'''
					case FBasicTypeId.UINT8: return '''TypeUtil::toQt(«objectName»)'''
					case FBasicTypeId.INT16: return '''TypeUtil::toQt(«objectName»)'''
					case FBasicTypeId.UINT16: return '''TypeUtil::toQt(«objectName»)'''
					case FBasicTypeId.INT32: return '''TypeUtil::toQt(«objectName»)'''
					case FBasicTypeId.UINT32: return '''TypeUtil::toQt(«objectName»)'''
					case FBasicTypeId.INT64: return '''TypeUtil::toQt(«objectName»)'''
					case FBasicTypeId.UINT64: return '''TypeUtil::toQt(«objectName»)'''
					case FBasicTypeId.FLOAT: return '''TypeUtil::toQt(«objectName»)'''
					case FBasicTypeId.DOUBLE: return objectName
					case FBasicTypeId.STRING: return '''TypeUtil::toQt(«objectName»)'''
					case FBasicTypeId.BYTE_BUFFER: return '''TypeUtil::toQByteArray(«objectName»)'''
					default: return objectName
				}
			}
		}
		else if (typedElement.isArray) {
			if (!convertComplexTypes || typedElement.type.isEnum) {
				return '''TypeUtil::toQt<«typedElement.type.typeName»>(«objectName»)'''
			}
			else {
				return '''TypeUtil::toQt<«stdTypeUtil.getTypeNameStd(typedElement.type.derived)», «typedElement.type.typeName»>(«objectName»)'''
			}
		}
		else if (typedElement.type.isEnum) {
			//fix as long as enums are not correctly integrated in the Qt/Std type conversion
			return objectName
		}
		else if (convertComplexTypes) {
			return typedElement.type.typeName + "::createQt(" + objectName + ")";
		}
		return objectName
	}

	def fromQTTypeToStdType(FTypedElement typedElement, String objectName) {
		fromQTTypeToStdType(typedElement, objectName, false)
	}

	def fromQTTypeToStdType(FTypedElement typedElement, String objectName, boolean convertComplexTypes) {
		if (typedElement.type.predefined != null &&
			typedElement.type.predefined != FBasicTypeId.UNDEFINED
		) {
			switch (typedElement.type.predefined) {
				case FBasicTypeId.BOOLEAN: 	if (typedElement.isArray)
												return '''TypeUtil::toStd(«objectName»)'''
											else
												return objectName
				case FBasicTypeId.INT8: return '''TypeUtil::toStdInt8(«objectName»)'''
				case FBasicTypeId.UINT8: return '''TypeUtil::toStdUInt8(«objectName»)'''
				case FBasicTypeId.INT16: return '''TypeUtil::toStdInt16(«objectName»)'''
				case FBasicTypeId.UINT16: return '''TypeUtil::toStdUInt16(«objectName»)'''
				case FBasicTypeId.INT32: return '''TypeUtil::toStdInt32(«objectName»)'''
				case FBasicTypeId.UINT32: return '''TypeUtil::toStdUInt32(«objectName»)'''
				case FBasicTypeId.INT64: return '''TypeUtil::toStdInt64(«objectName»)'''
				case FBasicTypeId.UINT64: return '''TypeUtil::toStdUInt64(«objectName»)'''
				case FBasicTypeId.FLOAT: return '''TypeUtil::toStdFloat(«objectName»)'''
				case FBasicTypeId.DOUBLE: 	if (typedElement.isArray)
												return '''TypeUtil::toStd(«objectName»)'''
											else
												return objectName
				case FBasicTypeId.STRING: return '''TypeUtil::toStd(«objectName»)'''
				case FBasicTypeId.BYTE_BUFFER: return '''TypeUtil::toStd(«objectName»)'''
				default: 	if (typedElement.isArray)
								return '''TypeUtil::toStd(«objectName»)'''
							else
								return objectName
			}
		}
		else if (typedElement.isArray) {
			if (!convertComplexTypes || typedElement.type.isEnum) {
				return '''TypeUtil::toStd<«typedElement.type.typeName»>(«objectName»)'''
			}
			else {
				return '''TypeUtil::toStd<«typedElement.type.typeName», «stdTypeUtil.getTypeNameStd(typedElement.type.derived)»>(«objectName»)'''
			}
		}
		else if (typedElement.type.isEnum){
			return objectName
		}
		else if (convertComplexTypes) {
			return typedElement.type.typeName + "::createStd(" + objectName + ")";
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

	def needsDatatypeConversion(FBasicTypeId basicType) {
		basicType === FBasicTypeId.STRING ||
		basicType === FBasicTypeId.INT8 ||
		basicType === FBasicTypeId.UINT8 ||
		basicType === FBasicTypeId.UINT16 ||
		basicType === FBasicTypeId.INT16 ||
		basicType === FBasicTypeId.UINT32 ||
		basicType === FBasicTypeId.INT32 ||
		basicType === FBasicTypeId.INT64 ||
		basicType === FBasicTypeId.UINT64 ||
		basicType === FBasicTypeId.BYTE_BUFFER ||
		basicType === FBasicTypeId.FLOAT
	}

	def needsDatatypeConversion(FTypedElement typedElement) {
		typedElement.isArray || 
		needsDatatypeConversion(typedElement.type.predefined)
	}

	def needsDatatypeConversion(FBroadcast broadcast) {
		for (outParam : broadcast.outputParameters) {
			if (needsDatatypeConversion(outParam)) {
				return true;
			}
		}
		return false;
	}

	override getTypeNameForList(FType datatype) {
		"QList<" + datatype.typeName + "> ";
	}

	override getTypeNameForList(FBasicTypeId datatype) {
		"QList<" + datatype.typeName + "> ";
	}

	override getIncludesFor(Iterable<FBasicTypeId> datatypes) {
		return new HashSet<String>;
	}
	
	override getIncludeForArray() {
		return null
	}

	override getIncludeOf(FType dataType) {
		val path = getPackagePathWithJoynrPrefix(dataType, "/")
		return path + "/" + dataType.joynrName + ".h";
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
}
