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
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "joynr/JoynrCommonExport.h"

#include <exception>
#include <string>
#include <memory>
#include "joynr/Variant.h"

namespace joynr
{

namespace exceptions
{

/**
 * @brief Base exception for all joynr exceptions.
 */
class JOYNRCOMMON_EXPORT JoynrException : public std::exception
{
public:
    /**
     * @brief Copy Constructor
     *
     * @param other The JoynrException to be copied from.
     */
    JoynrException(const JoynrException& other) noexcept;
    virtual ~JoynrException() noexcept;
    /**
     * @return The detail message string of the exception.
     */
    virtual const char* what() const noexcept;
    /**
     * @return The detail message string of the exception.
     */
    virtual const std::string getMessage() const noexcept;
    /**
     * return The typeName of the exception used for serialization.
     */
    virtual const std::string getTypeName() const;
    /**
     * @return A copy of the exception object.
     */
    virtual JoynrException* clone() const;
    /**
     * Equality operator
     */
    virtual bool operator==(const JoynrException& other) const;
    /**
     * @brief The typeName of the exception used for serialization.
     */
    static const std::string TYPE_NAME;
    /**
     * @brief Set the detail message of the exception.
     *
     * @param message Further description of the reported error (detail message).
     */
    virtual void setMessage(const std::string& message);

protected:
    /**
     * @brief the detail message of the exception.
     */
    std::string message;
    /**
     * @brief Constructor for a JoynrException without detail message.
     */
    JoynrException() noexcept;
    /**
     * @brief Constructor for a JoynrException with detail message.
     *
     * @param message Further description of the reported error (detail message).
     */
    JoynrException(const std::string& message) noexcept;
};

/**
 * @brief Base exception to report joynr runtime errors.
 */
class JOYNRCOMMON_EXPORT JoynrRuntimeException : public JoynrException
{
public:
    /**
     * @brief Constructor for a JoynrRuntimeException without detail message.
     */
    JoynrRuntimeException() noexcept = default;

    /**
     * @brief Constructor for a JoynrRuntimeException with detail message.
     *
     * @param message Further description of the reported runtime error
     */
    JoynrRuntimeException(const std::string& message) noexcept;
    virtual const std::string getTypeName() const;
    virtual JoynrRuntimeException* clone() const;
    /**
     * @brief The typeName used for serialization.
     */
    static const std::string TYPE_NAME;
};

/**
 * @brief Joynr exception to report timeouts.
 */
class JOYNRCOMMON_EXPORT JoynrTimeOutException : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a JoynrTimeOutException without detail message.
     */
    JoynrTimeOutException() noexcept = default;
    /**
     * @brief Constructor for a JoynrTimeOutException with detail message.
     *
     * @param message Further description of the reported timeout
     */
    JoynrTimeOutException(const std::string& message) noexcept;
    virtual const std::string getTypeName() const;
    virtual JoynrTimeOutException* clone() const;
    /**
     * @brief The typeName used for serialization.
     */
    static const std::string TYPE_NAME;
};

/**
 * @brief Joynr exception to report parse errors.
 */
class JOYNRCOMMON_EXPORT JoynrParseError : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a JoynrParseError with detail message.
     *
     * @param message Further description of the reported parse error
     */
    JoynrParseError(const std::string& message) noexcept;
};

/**
 * @brief Joynr exeption to report errors during discovery.
 */
class JOYNRCOMMON_EXPORT DiscoveryException : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a DiscoveryException without detail message.
     */
    DiscoveryException() noexcept = default;
    /**
     * @brief Constructor for a DiscoveryException with detail message.
     *
     * @param message Further description of the reported discovery error
     */
    DiscoveryException(const std::string& message) noexcept;
    virtual const std::string getTypeName() const;
    virtual DiscoveryException* clone() const;
    /**
     * @brief The typeName used for serialization.
     */
    static const std::string TYPE_NAME;
};

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
    MethodInvocationException() noexcept = default;
    /**
     * @brief Constructor for a MethodInvocationException with detail message.
     *
     * @param message Further description of the reported invocation error
     */
    MethodInvocationException(const std::string& message) noexcept;
    virtual const std::string getTypeName() const;
    virtual MethodInvocationException* clone() const;
    /**
     * @brief The typeName used for serialization.
     */
    static const std::string TYPE_NAME;
};

/**
 * @brief Joynr exception to report errors at the provider if no error enums are defined
 * in the corresponding Franca model file. It will also be used to wrap an transmit
 * unexpected exceptions which are thrown by the provider.
 */
class JOYNRCOMMON_EXPORT ProviderRuntimeException : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a ProviderRuntimeException without detail message.
     */
    ProviderRuntimeException() noexcept = default;
    /**
     * @brief Constructor for a ProviderRuntimeException with detail message.
     *
     * @param message Further description of the reported error
     */
    ProviderRuntimeException(const std::string& message) noexcept;
    virtual const std::string getTypeName() const;
    virtual ProviderRuntimeException* clone() const;
    /**
     * @brief The typeName used for serialization.
     */
    static const std::string TYPE_NAME;
};

/**
 * @brief Joynr exception to report missed periodic publications.
 */
class JOYNRCOMMON_EXPORT PublicationMissedException : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a PublicationMissedException without subscription ID.
     */
    PublicationMissedException() noexcept;
    /**
     * @brief Copy Constructor
     *
     * @param other The PublicationMissedException to copy from.
     */
    PublicationMissedException(const PublicationMissedException& other) noexcept;
    /**
     * @brief Constructor for a PublicationMissedException with subscription ID.
     *
     * @param subscriptionId The subscription ID of the subscription the missed
     * publication belongs to.
     */
    PublicationMissedException(const std::string& subscriptionId) noexcept;
    /**
     * @return The subscription ID of the subscription the missed publication
     * belongs to.
     */
    std::string getSubscriptionId() const noexcept;
    /**
     * @brief Set the subscriptionId of the exception.
     *
     * @param subscriptionId The subscription ID of the subscription the missed
     * publication belongs to.
     */
    virtual void setSubscriptionId(const std::string& subscriptionId) noexcept;
    virtual const std::string getTypeName() const;
    virtual PublicationMissedException* clone() const;
    /**
     * Equality operator
     */
    virtual bool operator==(const PublicationMissedException& other) const;
    /**
     * @brief The typeName used for serialization.
     */
    static const std::string TYPE_NAME;

private:
    std::string subscriptionId;
};

/**
 * @brief Joynr exception used to return error enums defined in the corresponding
 * Franca model file from provider to consumer.
 */
class JOYNRCOMMON_EXPORT ApplicationException : public JoynrException
{
public:
    /**
     * @brief Constructor for an ApplicationException without detail message.
     */
    ApplicationException() noexcept;

    /**
     * @brief Copy Constructor
     *
     * @param other The ApplicationException to copy from.
     */
    ApplicationException(const ApplicationException& other) noexcept;

    /**
     * @brief Constructor for an ApplicationException with detail message.
     *
     * @param message Description of the reported error
     * @param value The error Enum value
     * @param name The error Enum literal
     * @param typeName the type name of the error enumeration type (used for serialization)
     */
    ApplicationException(const std::string& message,
                         const Variant& value,
                         const std::string& name,
                         const std::string& typeName) noexcept;
    /**
     * @return The reported error Enum value.
     */
    template <class T>
    const T& getError() const;
    /**
     * @brief Set the error Enum value.
     *
     * @param value The error Enum value.
     */
    void setError(const Variant& value) noexcept;
    /**
     * @return The error Enum literal.
     */
    std::string getName() const noexcept;
    /**
     * @brief Set the error Enum literal.
     *
     * @param name the error Enum lital.
     */
    void setName(const std::string& name) noexcept;
    /**
     * @return The type name of the error enumeration.
     */
    std::string getErrorTypeName() const noexcept;

    /**
     * @brief Set the type name of the error enumeration.
     *
     * @param type name the type name of the error enumeration.
     */
    void setErrorTypeName(const std::string& typeName) noexcept;
    const std::string getTypeName() const;
    virtual ApplicationException* clone() const;
    /**
     * Equality operator
     */
    virtual bool operator==(const ApplicationException& other) const;
    /**
     * @brief The typeName of the exception used for serialization.
     */
    static const std::string TYPE_NAME;

private:
    Variant value;
    std::string name;
    std::string typeName;
};

template <class T>
const T& ApplicationException::getError() const
{
    return value.get<T>();
}

} // namespace exceptions

} // namespace joynr
#endif // EXCEPTIONS_H
