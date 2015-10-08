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
#ifndef SERIALIZER_H
#define SERIALIZER_H

namespace joynr
{

/**
 * @brief Interface to serializers
 */
class ISerializer
{
public:
    virtual void startObject() = 0;
    virtual void endObject() = 0;
    virtual void startArray() = 0;
    virtual void endArray() = 0;

    virtual void stringField(std::string name, std::string value) = 0;
    virtual void integerField(std::string name, int value) = 0;
    virtual void doubleField(std::string name, double value) = 0;

    virtual void arrayField(std::string name) = 0;
    virtual void objectField(std::string name) = 0;
};

} /* namespace joynr */
#endif // SERIALIZER_H

