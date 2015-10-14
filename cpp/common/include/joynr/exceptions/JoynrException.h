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
#include <QString>
#include <string>

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
    JoynrException(const JoynrException& other) throw();
    virtual ~JoynrException() throw();
    /**
     * @return The detail message string of the exception.
     */
    virtual const char* what() const throw();
    /**
     * @return The detail message string of the exception.
     */
    virtual const std::string getMessage() const throw();
    /**
     * return The typeName of the exception used for serialization.
     */
    virtual const std::string getTypeName() const;
    /**
     * @brief The typeName of the exception used for serialization.
     */
    static const std::string TYPE_NAME;

protected:
    /**
     * @brief the detail message of the exception.
     */
    std::string message;
    /**
     * @brief Constructor for a JoynrException without detail message.
     */
    JoynrException() throw();
    /**
     * @brief Constructor for a JoynrException with detail message.
     *
     * @param message Further description of the reported error (detail message).
     */
    JoynrException(const std::string& message) throw();
    /**
     * @brief Set the detail message of the exception.
     *
     * @param message Further description of the reported error (detail message).
     */
    virtual void setMessage(std::string message);
};

/**
 * @brief Base exception to report joynr runtime errors.
 */
class JOYNRCOMMON_EXPORT JoynrRuntimeException : public JoynrException
{
public:
    /**
     * @brief Constructor for a JoynrRuntimeException with detail message.
     *
     * @param message Further description of the reported runtime error
     */
    JoynrRuntimeException(const std::string& message) throw();
    virtual const std::string getTypeName() const;
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
     * @brief Constructor for a JoynrTimeOutException with detail message.
     *
     * @param message Further description of the reported timeout
     */
    JoynrTimeOutException(const std::string& message) throw();
    virtual const std::string getTypeName() const;
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
    JoynrParseError(const std::string& message) throw();
};

/**
 * @brief Joynr exeption to report errors during discovery.
 */
class JOYNRCOMMON_EXPORT DiscoveryException : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a DiscoveryException with detail message.
     *
     * @param message Further description of the reported discovery error
     */
    DiscoveryException(const std::string& message) throw();
    virtual const std::string getTypeName() const;
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
     * @brief Constructor for a MethodInvocationException with detail message.
     *
     * @param message Further description of the reported invocation error
     */
    MethodInvocationException(const std::string& message) throw();
    virtual const std::string getTypeName() const;
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
     * @brief Constructor for a ProviderRuntimeException with detail message.
     *
     * @param message Further description of the reported error
     */
    ProviderRuntimeException(const std::string& message) throw();
    virtual const std::string getTypeName() const;
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
     * @brief Copy Constructor
     *
     * @param other The PublicationMissedException to copy from.
     */
    PublicationMissedException(const PublicationMissedException& other) throw();
    /**
     * @brief Constructor for a PublicationMissedException with subscription ID.
     *
     * @param subscriptionId The subscription ID of the subscription the missed
     * publication belongs to.
     */
    PublicationMissedException(const std::string& subscriptionId) throw();
    /**
     * @return The subscription ID of the subscription the missed publication
     * belongs to.
     */
    std::string getSubscriptionId() const throw();
    virtual const std::string getTypeName() const;
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
     * @brief Copy Constructor
     *
     * @param other The ApplicationException to copy from.
     */
    ApplicationException(const ApplicationException& other) throw();
    /**
     * @brief Constructor for an ApplicationException without detail message.
     *
     * @param value The error Enum value
     * @param name The error Enum literal
     * @param typeName the typeName of the error enumeration type (used for serialization)
     */
    ApplicationException(const uint32_t& value,
                         const std::string& name,
                         const std::string& typeName) throw();
    /**
     * @brief Constructor for an ApplicationException with detail message.
     *
     * @param message Description of the reported error
     * @param value The error Enum value
     * @param name The error Enum literal
     * @param typeName the type name of the error enumeration type (used for serialization)
     */
    ApplicationException(const std::string& message,
                         const uint32_t& value,
                         const std::string& name,
                         const std::string& typeName) throw();
    /**
     * @return The reported error Enum value.
     */
    uint32_t getError() const throw();
    /**
     * @brief Set the error Enum value.
     *
     * @param value The error Enum value.
     */
    void setError(const uint32_t& value);
    /**
     * @return The error Enum literal.
     */
    std::string getName() const throw();
    /**
     * @return The type name of the error enumeration.
     */
    std::string getErrorTypeName() const throw();
    virtual const std::string getTypeName() const;
    /**
     * @brief The typeName of the exception used for serialization.
     */
    static const std::string TYPE_NAME;

private:
    uint32_t value;
    std::string name;
    std::string typeName;
};

} // namespace exceptions

} // namespace joynr
#endif // EXCEPTIONS_H
