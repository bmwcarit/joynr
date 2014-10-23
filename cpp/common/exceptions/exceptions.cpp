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
#include "joynr/exceptions.h"

namespace joynr
{

JoynrException::JoynrException() throw() : message("")
{
}

JoynrException::JoynrException(const std::string& what) throw() : message(what)
{
}

JoynrException::~JoynrException() throw()
{
}

const char* JoynrException::what() const throw()
{
    return message.c_str();
}

JoynrTimeOutException::JoynrTimeOutException() throw()
{
}

JoynrTimeOutException::JoynrTimeOutException(const std::string& what) throw() : JoynrException(what)
{
}

JoynrParseError::JoynrParseError(const std::string& what) throw() : JoynrException(what)
{
}

JoynrArbitrationException::JoynrArbitrationException(const std::string& what) throw()
        : JoynrException(what)
{
}

JoynrArbitrationFailedException::JoynrArbitrationFailedException(const std::string& what) throw()
        : JoynrException(what)
{
}

JoynrArbitrationTimeOutException::JoynrArbitrationTimeOutException(const std::string& what) throw()
        : JoynrException(what)
{
}

JoynrRuntimeException::JoynrRuntimeException(const std::string& what) throw() : JoynrException(what)
{
}

void JoynrException::setMessage(std::string what)
{
    this->message = what;
}

} // namespace joynr
