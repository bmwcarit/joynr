/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2017 BMW Car IT GmbH
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
#ifndef SERIALIZERTRAITS_H
#define SERIALIZERTRAITS_H

namespace joynr
{
namespace serializer
{

// this is a traits class which is specialized for pairs of Input and Output archive templates
// it provides a link between the id-string and the tag of the Input/Output archives
// as well as a link between that tag and a specific Deserializable
template <typename ArchiveTag>
struct SerializerTraits;

} // namespace serializer
} // namespace joynr

#endif // SERIALIZERTRAITS_H
