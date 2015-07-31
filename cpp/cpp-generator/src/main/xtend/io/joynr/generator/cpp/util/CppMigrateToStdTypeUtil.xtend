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
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FType

class CppMigrateToStdTypeUtil extends CppTypeUtil {
	@Inject
	private QtTypeUtil qtTypeUtil

	@Inject
	private CppStdTypeUtil cppStdTypeUtil

	override getTypeName(FBasicTypeId datatype) {
		cppStdTypeUtil.getTypeName(datatype)
	}

	override getTypeNameForList(FType datatype) {
		"std::vector<" + datatype.typeName + "> ";
	}

	override getTypeNameForList(FBasicTypeId datatype) {
		cppStdTypeUtil.getTypeNameForList(datatype)
	}

	override getTypeName(FType dataType) {
/*
		if (dataType.isEnum) {
			qtTypeUtil.getTypeName(dataType)
		}
		else {
			cppStdTypeUtil.getTypeName(dataType)
		}
*/
		qtTypeUtil.getTypeName(dataType)
	}

	override getIncludeForArray() {
		cppStdTypeUtil.getIncludeForArray
	}

	def getIncludeForString() {
		cppStdTypeUtil.includeForString
	}

	def getIncludeForInteger() {
		cppStdTypeUtil.includeForInteger
	}

	override getIncludesFor(Iterable<FBasicTypeId> datatypes) {
		cppStdTypeUtil.getIncludesFor(datatypes)
	}

	override String getIncludeOf(FType dataType) {
		qtTypeUtil.getIncludeOf(dataType)
	}
}