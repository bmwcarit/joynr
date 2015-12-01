package io.joynr.generator.cpp.communicationmodel.serializer
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

import io.joynr.generator.cpp.util.CppStdTypeUtil
import io.joynr.generator.cpp.util.JoynrCppGeneratorExtensions
import io.joynr.generator.cpp.util.TemplateBase
import io.joynr.generator.templates.EnumTemplate
import io.joynr.generator.templates.util.NamingUtil
import javax.inject.Inject
import org.franca.core.franca.FEnumerationType

class EnumSerializerCppTemplate implements EnumTemplate{

	@Inject
	private extension JoynrCppGeneratorExtensions

	@Inject
	private extension CppStdTypeUtil

	@Inject
	private extension NamingUtil

	@Inject
	private extension TemplateBase

	override generate(FEnumerationType type)
'''
«val joynrName = type.joynrName»
«val typeName = type.typeName»
«warning»
#include "«type.includeOfSerializer»"
#include "joynr/SerializerRegistry.h"
#include "joynr/JoynrTypeId.h"

#include <string>
#include <utility>
#include <algorithm>

namespace joynr
{

// Register the «typeName» type id (_typeName value) and serializer/deserializer
static const bool is«joynrName»SerializerRegistered =
		SerializerRegistry::registerEnum<«typeName»>("«type.typeName.replace("::", ".")»");

// Deserializes a «joynrName»
template <>
void EnumDeserializer<«typeName»>::deserialize(«typeName»& «joynrName.toFirstLower», IValue& value)
{
	std::string text = value;

	«FOR literal : type.enumElementsAndBaseEnumElements»
	if (text == "«literal.joynrName»") {
		«joynrName.toFirstLower» = «typeName»::«literal.joynrName»;
	} else
	«ENDFOR»
	{
		throw std::invalid_argument("Unknown enum value");
	}
}

template <>
void ClassSerializer<«typeName»>::serialize(const «typeName»& «joynrName.toFirstLower», std::ostream& stringstream)
{
	«val nameOfContainingEnum = buildPackagePath(type, "::", true) + joynrName»;

	stringstream << "\""<< «nameOfContainingEnum»::getLiteral(«joynrName.toFirstLower») << "\"";
}

} /* namespace joynr */

'''
}
