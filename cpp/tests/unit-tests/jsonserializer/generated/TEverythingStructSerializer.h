/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2015 BMW Car IT GmbH
 * %%
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
 * #L%
 */
#ifndef TEVERYTHINGSTRUCTSERIALIZER_H
#define TEVERYTHINGSTRUCTSERIALIZER_H

#include "joynr/ClassDeserializer.h"
#include "joynr/ClassSerializer.h"
#include "joynr/types/TestTypes/TEverythingStruct.h"

#include <ostream>

namespace joynr {

using TEverythingStruct = joynr::types::TestTypes::TEverythingStruct;
using TEnum = joynr::types::TestTypes::TEnum;
using Word = joynr::types::TestTypes::Word;
using Vowel = joynr::types::TestTypes::Vowel;

// Serializes a TEverythingStruct
template <>
void ClassSerializer<TEverythingStruct>::serialize(const TEverythingStruct& everythingStruct, std::ostream& stream);

// Deserializes a TEverythingStruct
template <>
void ClassDeserializer<TEverythingStruct>::deserialize(TEverythingStruct& everythingStruct, IObject& object);

// Serialize TEnum
template <>
void ClassSerializer<TEnum::Enum>::serialize(const TEnum::Enum &tEnum, std::ostream &stream);

// Serialize Word
template <>
void ClassSerializer<Word>::serialize(const Word &word, std::ostream &stream);

// Deserializes a Word
template <>
void ClassDeserializer<Word>::deserialize(Word& word, IObject& object);

// Serialize Vowels
template <>
void ClassSerializer<Vowel::Enum>::serialize(const Vowel::Enum& vowelEnum, std::ostream& stream);

} /* namespace joynr */

#endif // TEVERYTHINGSTRUCTSERIALIZER_H
