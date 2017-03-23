package io.joynr.generator.cpp.util
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
import java.util.HashSet
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FCompoundType
import org.franca.core.franca.FStructType
import org.franca.core.franca.FType
import org.franca.core.franca.FTypedElement
import org.franca.core.franca.FUnionType

class CppStdTypeUtil extends CppTypeUtil {

	@Inject
	private extension JoynrCppGeneratorExtensions

	override getTypeName(FBasicTypeId datatype) {
		switch datatype {
			case FBasicTypeId::BOOLEAN: "bool"
			case FBasicTypeId::INT8: "std::int8_t"
			case FBasicTypeId::UINT8: "std::uint8_t"
			case FBasicTypeId::INT16: "std::int16_t"
			case FBasicTypeId::UINT16: "std::uint16_t"
			case FBasicTypeId::INT32: "std::int32_t"
			case FBasicTypeId::UINT32: "std::uint32_t"
			case FBasicTypeId::INT64: "std::int64_t"
			case FBasicTypeId::UINT64: "std::uint64_t"
			case FBasicTypeId::FLOAT: "float"
			case FBasicTypeId::DOUBLE: "double"
			case FBasicTypeId::STRING: "std::string"
			case FBasicTypeId::BYTE_BUFFER: "joynr::ByteBuffer"
			default: throw new IllegalArgumentException("Unsupported basic type: " + datatype.getName)
		}
	}

	def getByteBufferElementType() {
		FBasicTypeId.UINT8.typeName
	}

	override getTypeNameForList(FType datatype) {
		"std::vector<" + datatype.typeName + "> ";
	}

	override getTypeNameForList(FBasicTypeId datatype) {
		"std::vector<" + datatype.typeName + "> ";
	}

	override getGenerationTypeName(FType datatype) {
		datatype.joynrName
	}

	override getTypeName(FType datatype) {
		var typeName = buildPackagePath(datatype, "::", true) + "::" + datatype.joynrName;
		if (isEnum(datatype)){
			typeName += "::" + getNestedEnumName();
		}
		return typeName
	}

	override getIncludeForArray() {
		"<vector>"
	}

	override def getIncludeForString() {
		"<string>"
	}

	def getIncludeForInteger() {
		"<cstdint>"
	}

	def getIncludeForByteBuffer() {
		"\"joynr/ByteBuffer.h\""
	}

	override getIncludesFor(Iterable<FBasicTypeId> datatypes) {
		var includes = new HashSet<String>;
		if (datatypes.exists[type | type == FBasicTypeId.STRING]) {
			includes.add(includeForString);
		}
		if (datatypes.exists[type | type == FBasicTypeId.BYTE_BUFFER]) {
			includes.add(includeForByteBuffer);
		}
		if (datatypes.exists[type | type == FBasicTypeId.INT8  ||
									type == FBasicTypeId.UINT8 ||
									type == FBasicTypeId.INT16 ||
									type == FBasicTypeId.UINT16||
									type == FBasicTypeId.INT32 ||
									type == FBasicTypeId.UINT32||
									type == FBasicTypeId.INT64 ||
									type == FBasicTypeId.UINT64
		]) {
			includes.add(includeForInteger)
		}
		return includes;
	}

	override String getIncludeOf(FType dataType) {
		getIncludeOf(dataType, "")
	}

	private def String getIncludeOf(FType dataType, String nameSuffix) {
		var path = getPackagePathWithJoynrPrefix(dataType, "/")
		if (dataType.isPartOfTypeCollection) {
			path += "/" + dataType.typeCollectionName
		}
		return "\"" + path + "/" + dataType.joynrName + nameSuffix + ".h\"";
	}

	override getDefaultValue(FTypedElement element) {
		if (element.type.predefined == FBasicTypeId.BYTE_BUFFER) {
			return "";
		}
		else {
			super.getDefaultValue(element)
		}
	}

	def FStructType getRootType(FStructType datatype)
	{
		if (datatype.base != null) {
			return datatype.base.rootType
		}
		return datatype
	}

	def FUnionType getRootType(FUnionType datatype)
	{
		if (datatype.base != null) {
			return datatype.base.rootType
		}
		return datatype
	}

	def FCompoundType getRootType(FCompoundType datatype)
	{
		if (datatype instanceof FStructType) {
			return datatype.rootType
		} else if (datatype instanceof FUnionType) {
			return datatype.rootType
		}
		throw new IllegalStateException("CppStdTypeUtil.getRootType: unknown type "
										+ datatype.class.simpleName
		);
	}

	def getForwardDeclaration(FType datatype)'''
«getNamespaceStarter(datatype, true)»
class «(datatype).joynrName»;
«getNamespaceEnder(datatype, true)»
'''

}
