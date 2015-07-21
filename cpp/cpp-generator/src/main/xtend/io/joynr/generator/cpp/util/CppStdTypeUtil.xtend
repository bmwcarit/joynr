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
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FType

class CppStdTypeUtil extends CppTypeUtil {

	@Inject
	private extension JoynrCppGeneratorExtensions

	override getTypeName(FBasicTypeId datatype) {
		switch datatype {
			case FBasicTypeId::BOOLEAN: "bool"
			case FBasicTypeId::INT8: "int8_t"
			case FBasicTypeId::UINT8: "uint8_t"
			case FBasicTypeId::INT16: "int16_t"
			case FBasicTypeId::UINT16: "uint16_t"
			case FBasicTypeId::INT32: "int32_t"
			case FBasicTypeId::UINT32: "uint32_t"
			case FBasicTypeId::INT64: "int64_t"
			case FBasicTypeId::UINT64: "uint64_t"
			case FBasicTypeId::FLOAT: "float"
			case FBasicTypeId::DOUBLE: "double"
			case FBasicTypeId::STRING: "std::string"
			case FBasicTypeId::BYTE_BUFFER: "std::vector<uint8_t>"
			default: throw new IllegalArgumentException("Unsupported basic type: " + datatype.getName)
		}
	}

	override getTypeNameForList(FType datatype) {
		"std::vector<" + datatype.typeName + "> ";
	}

	override getTypeNameForList(FBasicTypeId datatype) {
		"std::vector<" + datatype.typeName + "> ";
	}

	override getTypeName(FType datatype) {
		if (isEnum(datatype)){
		val packagepath = buildPackagePath(datatype, "::");
			return  packagepath + datatype.joynrName+ "::" + getNestedEnumName();
		}
		else{
			datatype.typeNameStd
		}
	}

	def getTypeNameStd(FType datatype) {
		val packagepath = buildPackagePath(datatype, "::");
		return  packagepath + datatype.joynrNameStd  //if we don't know the type, we have to assume its a complex datatype defined somewhere else.
	}

	override getIncludeForArray() {
		"<vector>"
	}

	def getIncludeForString() {
		"<string>"
	}

	def getIncludeForInteger() {
		"<stdint.h>"
	}

	override getIncludesFor(Iterable<FBasicTypeId> datatypes) {
		var includes = new HashSet<String>;
		if (datatypes.exists[type | type == FBasicTypeId.STRING]) {
			includes.add(includeForString);
		}
		if (datatypes.exists[type | type == FBasicTypeId.BYTE_BUFFER]) {
			includes.add(includeForArray);
		}
		if (datatypes.exists[type | type == FBasicTypeId.INT8  ||
									type == FBasicTypeId.UINT8 ||
									type == FBasicTypeId.INT16 ||
									type == FBasicTypeId.UINT16||
									type == FBasicTypeId.INT32 ||
									type == FBasicTypeId.UINT32||
									type == FBasicTypeId.INT64 ||
									type == FBasicTypeId.UINT64||
									type == FBasicTypeId.BYTE_BUFFER
		]) {
			includes.add(includeForInteger)
		}
		return includes;
	}

	override String getIncludeOf(FType dataType) {
		if (dataType.isEnum) {
			val path = getPackagePathWithJoynrPrefix(dataType, "/")
			return path + "/" + dataType.joynrName + ".h";
		}
		else {
			dataType.includeOfStd
		} 
	}

	def getIncludeOfStd(FType dataType) {
		var path = getPackagePathWithJoynrPrefix(dataType, "/")
		if (dataType.isPartOfTypeCollection) {
			path += "/" + dataType.typeCollectionName
		}
		return path + "/" + dataType.joynrNameStd + ".h";
	}
}