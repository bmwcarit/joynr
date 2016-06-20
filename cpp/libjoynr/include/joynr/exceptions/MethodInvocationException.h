/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#ifndef METHODINVOCATIONEXCEPTION_H
#define METHODINVOCATIONEXCEPTION_H

#include "joynr/JoynrCommonExport.h"

#include <chrono>
#include <exception>
#include <string>
#include "joynr/exceptions/JoynrException.h"
#include "joynr/Variant.h"
#include "joynr/types/Version.h"

namespace joynr
{

using joynr::types::Version;

namespace exceptions
{

/**
 * @brief Joynr exception class to report error during method invocations (RPC) at a provider
 * ("no such method", invalid arguments, etc.)
 */
class JOYNRCOMMON_EXPORT MethodInvocationException : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a MethodInvocationException without detail message.
     */
    MethodInvocationException() noexcept;
    /**
     * @brief Constructor for a MethodInvocationException with detail message.
     *
     * @param message Further description of the reported invocation error
     */
    explicit MethodInvocationException(const std::string& message) noexcept;
    explicit MethodInvocationException(const std::string& message,
                                       const Version& providerVersion) noexcept;
    const std::string getTypeName() const override;
    MethodInvocationException* clone() const override;
    /**
     * @brief The typeName used for serialization and logging.
     */
    static const std::string TYPE_NAME;

private:
    Version providerVersion;
};

} // namespace exceptions

} // namespace joynr
#endif // METHODINVOCATIONEXCEPTION_H
