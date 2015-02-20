/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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
#include "cluster-controller/httpnetworking/ReadData.h"

namespace joynr
{

ReadData::ReadData() : byteArray()
{
}

ReadData::ReadData(const QString& dataQString) : byteArray(dataQString.toLatin1())
{
}

size_t ReadData::getLen() const
{
    return byteArray.length();
}

const char* ReadData::getData() const
{
    return byteArray.constData();
}

bool ReadData::isEmpty() const
{
    return byteArray.isEmpty() || byteArray.length() == 0;
};

} // namespace joynr
