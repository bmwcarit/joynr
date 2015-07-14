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

import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FType

class CppMigrateToStdTypeUtil extends CppTypeUtil {

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
		"QList<" + datatype.typeName + "> ";
	}

	override getTypeNameForList(FBasicTypeId datatype) {
		"QList<" + datatype.typeName + "> ";
	}
}