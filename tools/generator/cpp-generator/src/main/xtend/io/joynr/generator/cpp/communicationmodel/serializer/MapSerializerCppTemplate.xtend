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
 *	  http://www.apache.org/licenses/LICENSE-2.0
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
import io.joynr.generator.templates.MapTemplate
import io.joynr.generator.templates.util.NamingUtil
import java.util.UUID
import javax.inject.Inject
import org.franca.core.franca.FBasicTypeId
import org.franca.core.franca.FMapType
import org.franca.core.franca.FTypeRef

class MapSerializerCppTemplate implements MapTemplate{

	@Inject private extension JoynrCppGeneratorExtensions

	@Inject private extension CppStdTypeUtil

	@Inject private extension NamingUtil

	@Inject private extension TypeSerializerCppTemplate

	@Inject private extension TemplateBase

	override generate(FMapType type)
'''
«val joynrName = type.joynrName»
«val typeName = type.typeName»
«warning»
#include "«type.includeOfSerializer»"
#include "joynr/ArraySerializer.h"
#include "joynr/PrimitiveDeserializer.h"
#include "joynr/MapSerializer.h"
#include "joynr/SerializerRegistry.h"
#include "joynr/Variant.h"
#include "joynr/JoynrTypeId.h"

#include <string>
#include <utility>
#include <algorithm>

namespace joynr
{

using namespace «getPackagePathWithJoynrPrefix(type, "::", true)»;

«var fqJoynrName = typeName.replace("::", ".")»
// Register the «joynrName» type id (_typeName value) and serializer/deserializer
static const bool is«joynrName»SerializerRegistered =
		SerializerRegistry::registerType<«joynrName»>("«fqJoynrName»");

template <>
void ClassDeserializer<«joynrName»>::deserialize(«joynrName» &«joynrName.toFirstLower»Var, IObject &object)
{
	while (object.hasNextField()) {
		IField& field = object.nextField();
		if (field.key().isString() && field.name() == "_typeName") {
			continue;
		}
		«val deserializeMapEntry = [FTypeRef typeRef, String varName | '''
			«IF typeRef.isPrimitive»
				«deserializePrimitiveValue(typeRef.predefined, varName, "field." + varName + "()")»
			«ELSE»
				«typeRef.typeName» «varName»;
				«val complexType = typeRef.derived»
				«val deserializerType = complexType.deserializer»
				«deserializerType»<«complexType.typeName»>::deserialize(«varName», field.«varName»());
		«ENDIF»
		''']»
		«deserializeMapEntry.apply(type.keyType, "key")»
		«deserializeMapEntry.apply(type.valueType, "value")»
		«joynrName.toFirstLower»Var.insert({key, value});
	}
}

template <>
void ClassSerializer<«joynrName»>::serialize(const «joynrName» &«joynrName.toFirstLower»Var, std::ostream& stream)
{
	MapSerializer::serialize<«type.keyType.typeName», «type.valueType.typeName»>(
				"«fqJoynrName»",
				«joynrName.toFirstLower»Var,
				stream);

}

} // namespace joynr

'''

def deserializePrimitiveValue(FBasicTypeId basicType, String varName, String fieldValue) {
 
	var deserializedValue = basicType.typeName + " " + varName + " = " + fieldValue
	switch basicType {
		case BYTE_BUFFER : return '''
		«val randomName = "converted" + UUID::randomUUID.toString.substring(0, 5)»
		IArray& array = «fieldValue»;
		auto&& «randomName» = convertArray<uint8_t>(array, convertUIntType<uint8_t>);
		«basicType.typeName» «varName» = std::forward<std::vector<uint8_t>>(«randomName.toFirstUpper»);
		'''
		case STRING : '''
		std::string «varName»;
		PrimitiveDeserializer<std::string>::deserialize(«varName», «fieldValue»);
		'''
		case BOOLEAN : return deserializedValue + ".getBool();"
		case INT8 : return deserializedValue + ".getIntType<int8_t>();"
		case INT16 : return deserializedValue + ".getIntType<int16_t>();"
		case INT32 : return deserializedValue + ".getIntType<int32_t>();"
		case INT64 : return deserializedValue + ".getIntType<int64_t>();"
		case UINT8 : return deserializedValue + ".getIntType<uint8_t>();"
		case UINT16 : return deserializedValue + ".getIntType<uint16_t>();"
		case UINT32 : return deserializedValue + ".getIntType<uint32_t>();"
		case UINT64 : return deserializedValue + ".getIntType<uint64_t>();"
		case FLOAT : return deserializedValue + ".getDoubleType<float>();"
		case DOUBLE : return deserializedValue + ".getDoubleType<double>();"
		default: throw new IllegalStateException("Type for varName " + varName + " could not be resolved")
	}
}

}
