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
#include "joynr/Variant.h"
#include <cstdint>
#include <string>
#include <vector>

namespace joynr
{

joynr_logging::Logger* Variant::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "Variant");

// Register basic types to be able to put them in Variant
static const bool isInt8Registered = Variant::registerType<int8_t>("Int8");
static const bool isUInt8Registered = Variant::registerType<uint8_t>("UInt8");
static const bool isInt16Registered = Variant::registerType<int16_t>("Int16");
static const bool isUInt16Registered = Variant::registerType<uint16_t>("UInt16");
static const bool isInt32Registered =
        Variant::registerType<int32_t>("Int32"); // ambiguous with "Integer", if we use
                                                 // this one only then Dispatcher should
                                                 // be changed
static const bool isUInt32Registered = Variant::registerType<uint32_t>("UInt32");
static const bool isInt64Registered = Variant::registerType<int64_t>("Int64");
static const bool isUInt64Registered = Variant::registerType<uint64_t>("UInt64");
static const bool isDoubleRegistered = Variant::registerType<double>("Double");
static const bool isFloatRegistered = Variant::registerType<float>("Float");
static const bool isBooleanRegistered = Variant::registerType<bool>("Boolean");
static const bool isStringRegistered = Variant::registerType<std::string>("String");

static const bool isVectorVariantRegistered =
        Variant::registerType<std::vector<Variant>>("VectorVariant");
} // end namespace joynr
