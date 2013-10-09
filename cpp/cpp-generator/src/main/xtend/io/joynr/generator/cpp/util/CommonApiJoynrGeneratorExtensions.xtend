package io.joynr.generator.cpp.util
/*
 * !!!
 *
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.generator.util.JoynrGeneratorExtensions
import org.franca.core.franca.FAttribute
import org.franca.core.franca.FBasicTypeId

abstract class CommonApiJoynrGeneratorExtensions extends JoynrGeneratorExtensions {
	override getPrimitiveTypeName(FBasicTypeId basicType) {
		switch basicType {
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
			case FBasicTypeId::BYTE_BUFFER: "CommonAPI::ByteBuffer"
			default: throw new IllegalArgumentException("Unsupported basic type: " + basicType.name)
        }
		// francaExtensions.getPrimitiveTypeName(basicType)
	}
	
    override isReadonly(FAttribute fAttribute) { fAttribute.readonly!=null }

    override isObservable(FAttribute fAttribute) { !(fAttribute.noSubscriptions!=null) }

}