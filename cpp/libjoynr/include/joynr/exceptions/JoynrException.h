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
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <chrono>
#include <exception>
#include <string>

#include <boost/optional.hpp>

#include "joynr/JoynrExport.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

namespace exceptions
{

/**
 * @brief Base exception for all joynr exceptions.
 */
class JOYNR_EXPORT JoynrException : public std::exception
{
public:
    /**
     * @brief Copy Constructor
     *
     * @param other The JoynrException to be copied from.
     */
    JoynrException(const JoynrException& other) = default;
    ~JoynrException() noexcept override = default;
    /**
     * @return The detail message string of the exception.
     */
    const char* what() const noexcept override;
    /**
     * @return The detail message string of the exception.
     */
    virtual std::string getMessage() const noexcept;
    /**
     * return The typeName of the exception used for serialization and logging.
     */
    virtual const std::string& getTypeName() const;
    /**
     * @return A copy of the exception object.
     */
    virtual JoynrException* clone() const = 0;
    /**
     * Equality operator
     */
    bool operator==(const JoynrException& other) const;
    /**
     * @brief The typeName of the exception used for serialization and logging.
     */
    static const std::string& TYPE_NAME();
    /**
     * @brief Set the detail message of the exception.
     *
     * @param message Further description of the reported error (detail message).
     */
    virtual void setMessage(const std::string& message);

    template <typename Archive>
    void serialize(Archive& ar)
    {
        ar(muesli::make_nvp("detailMessage", _message));
    }

protected:
    /**
     * @brief the detail message of the exception.
     */
    boost::optional<std::string> _message;
    /**
     * @brief Constructor for a JoynrException without detail message.
     */
    JoynrException() noexcept;
    /**
     * @brief Constructor for a JoynrException with detail message.
     *
     * @param message Further description of the reported error (detail message).
     */
    explicit JoynrException(const std::string& message) noexcept;
};

/**
 * @brief Base exception to report joynr runtime errors.
 */
class JOYNR_EXPORT JoynrRuntimeException : public JoynrException
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
    explicit JoynrRuntimeException(const std::string& message) noexcept;
    const std::string& getTypeName() const override;
    JoynrRuntimeException* clone() const override;
    /**
     * @brief The typeName used for serialization and logging.
     */
    static const std::string& TYPE_NAME();

    template <typename Archive>
    void serialize(Archive& ar)
    {
        ar(muesli::BaseClass<JoynrException>(this));
    }
};

class JOYNR_EXPORT JoynrConfigurationException : public JoynrRuntimeException
{
public:
    explicit JoynrConfigurationException(const std::string& message) noexcept;

    const std::string& getTypeName() const override;
    JoynrConfigurationException* clone() const override;

    static const std::string& TYPE_NAME();
};

/**
 * @brief Joynr exception to report timeouts.
 */
class JOYNR_EXPORT JoynrTimeOutException : public JoynrRuntimeException
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
    explicit JoynrTimeOutException(const std::string& message) noexcept;
    const std::string& getTypeName() const override;
    JoynrTimeOutException* clone() const override;
    /**
     * @brief The typeName used for serialization and logging.
     */
    static const std::string& TYPE_NAME();
};

/**
 * @brief Joynr exception to report unresolvable send errors
 */
class JOYNR_EXPORT JoynrMessageNotSentException : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a JoynrMessageNotSentException without detail message.
     */
    JoynrMessageNotSentException() noexcept = default;
    /**
     * @brief Constructor for a JoynrMessageNotSentException with detail message.
     *
     * @param message reason why the message could not be sent
     */
    explicit JoynrMessageNotSentException(const std::string& message) noexcept;
    const std::string& getTypeName() const override;
    JoynrMessageNotSentException* clone() const override;
    /**
     * @brief The typeName used for serialization and logging.
     */
    static const std::string& TYPE_NAME();
};

/**
 * @brief Joynr exception to report send errors which might be solved after some delay.
 */
class JOYNR_EXPORT JoynrDelayMessageException : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a JoynrDelayMessageException without detail message and default delay.
     */
    JoynrDelayMessageException() noexcept;
    /**
     * @brief Copy Constructor
     *
     * @param other The JoynrDelayMessageException to copy from.
     */
    JoynrDelayMessageException(const JoynrDelayMessageException& other) = default;
    /**
     * @brief Constructor for a JoynrDelayMessageException with detail message and default delay.
     *
     * @param message reason why the message is being delayed
     */
    explicit JoynrDelayMessageException(const std::string& message) noexcept;
    /**
     * @brief Constructor for a JoynrDelayMessageException with detail message and delay.
     *
     * @param message reason why the message is being delayed
     */
    explicit JoynrDelayMessageException(const std::chrono::milliseconds delayMs,
                                        const std::string& message) noexcept;
    /**
     * @return The delay in milliseconds.
     */
    std::chrono::milliseconds getDelayMs() const noexcept;
    /**
     * @brief Set the delay.
     *
     * @param delayMs The delay in milliseconds.
     */
    virtual void setDelayMs(const std::chrono::milliseconds& delayMs) noexcept;
    const std::string& getTypeName() const override;
    JoynrDelayMessageException* clone() const override;
    /**
     * Equality operator
     */
    bool operator==(const JoynrDelayMessageException& other) const;

    /**
     * @brief The typeName used for serialization and logging.
     */
    static const std::string& TYPE_NAME();
    static const std::chrono::milliseconds DEFAULT_DELAY_MS;

    template <typename Archive>
    void serialize(Archive& ar)
    {
        ar(muesli::BaseClass<JoynrRuntimeException>(this),
           muesli::make_nvp("delayMs", _delayMs.count()));
    }

private:
    std::chrono::milliseconds _delayMs;
};

/**
 * @brief Joynr exception to report parse errors.
 */
class JOYNR_EXPORT JoynrParseError : public JoynrRuntimeException
{
public:
    /**
     * @brief Constructor for a JoynrParseError with detail message.
     *
     * @param message Further description of the reported parse error
     */
    explicit JoynrParseError(const std::string& message) noexcept;
};

/**
 * @brief Joynr exception to report errors during discovery.
 */
class JOYNR_EXPORT DiscoveryException : public JoynrRuntimeException
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
    explicit DiscoveryException(const std::string& message) noexcept;
    const std::string& getTypeName() const override;
    DiscoveryException* clone() const override;
    /**
     * @brief The typeName used for serialization and logging.
     */
    static const std::string& TYPE_NAME();
};

/**
 * @brief Joynr exception to report errors at the provider if no error enums are defined
 * in the corresponding Franca model file. It will also be used to wrap an transmit
 * unexpected exceptions which are thrown by the provider.
 */
class JOYNR_EXPORT ProviderRuntimeException : public JoynrRuntimeException
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
    explicit ProviderRuntimeException(const std::string& message) noexcept;
    const std::string& getTypeName() const override;
    ProviderRuntimeException* clone() const override;
    /**
     * @brief The typeName used for serialization and logging.
     */
    static const std::string& TYPE_NAME();
};

/**
 * @brief Joynr exception to report missed periodic publications.
 */
class JOYNR_EXPORT PublicationMissedException : public JoynrRuntimeException
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
    PublicationMissedException(const PublicationMissedException& other) = default;
    /**
     * @brief Constructor for a PublicationMissedException with subscription ID.
     *
     * @param subscriptionId The subscription ID of the subscription the missed
     * publication belongs to.
     */
    explicit PublicationMissedException(const std::string& subscriptionId) noexcept;
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
    const std::string& getTypeName() const override;
    PublicationMissedException* clone() const override;
    /**
     * Equality operator
     */
    bool operator==(const PublicationMissedException& other) const;
    /**
     * @brief The typeName used for serialization and logging.
     */
    static const std::string& TYPE_NAME();

    template <typename Archive>
    void serialize(Archive& ar)
    {
        ar(muesli::BaseClass<JoynrRuntimeException>(this),
           muesli::make_nvp("subscriptionId", _subscriptionId));
    }

private:
    std::string _subscriptionId;
};

class ApplicationExceptionError
{
public:
    ApplicationExceptionError() : _name()
    {
    }
    explicit ApplicationExceptionError(const std::string& name) : _name(name)
    {
    }
    explicit ApplicationExceptionError(std::string&& name) : _name(std::move(name))
    {
    }
    // shall be polymorphic AND abstract
    virtual ~ApplicationExceptionError() = 0;
    template <typename Archive>
    void serialize(Archive& ar)
    {
        ar(muesli::make_nvp("name", _name));
    }
    const std::string& getName() const
    {
        return _name;
    }

private:
    std::string _name;
};

inline ApplicationExceptionError::~ApplicationExceptionError() = default;

/**
 * @brief Joynr exception used to return error enums defined in the corresponding
 * Franca model file from provider to consumer.
 */
class JOYNR_EXPORT ApplicationException : public JoynrException
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
    ApplicationException(const ApplicationException& other) = default;

    /**
     * @brief Constructor for an ApplicationException with detail message.
     *
     * @param message Description of the reported error
     * @param name The error Enum literal
     * @param typeName the type name of the error enumeration type (used for serialization and
     * logging)
     */
    ApplicationException(const std::string& message,
                         std::shared_ptr<ApplicationExceptionError> error) noexcept;

    /**
     * @return The error Enum literal.
     */
    std::string getName() const noexcept;

    const std::string& getTypeName() const override;
    ApplicationException* clone() const override;
    /**
     * Equality operator
     */
    bool operator==(const ApplicationException& other) const;
    /**
     * @brief The typeName of the exception used for serialization and logging.
     */
    static const std::string& TYPE_NAME();

    template <typename ErrorEnum>
    ErrorEnum getError() const
    {
        using Wrapper = typename muesli::EnumTraits<ErrorEnum>::Wrapper;
        return Wrapper::getEnum(getName());
    }

    template <typename Archive>
    void serialize(Archive& ar)
    {
        ar(muesli::BaseClass<JoynrException>(this), muesli::make_nvp("error", _error));
    }

private:
    // FIXME should be a unique_ptr, but cannot be due to throwJoynrException
    std::shared_ptr<ApplicationExceptionError> _error;
};

} // namespace exceptions

} // namespace joynr

MUESLI_REGISTER_TYPE(joynr::exceptions::JoynrException, "joynr.exceptions.JoynrException")
MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::exceptions::JoynrRuntimeException,
                                 joynr::exceptions::JoynrException,
                                 "joynr.exceptions.JoynrRuntimeException")
MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::exceptions::JoynrTimeOutException,
                                 joynr::exceptions::JoynrRuntimeException,
                                 "joynr.exceptions.JoynrTimeOutException")
MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::exceptions::JoynrMessageNotSentException,
                                 joynr::exceptions::JoynrRuntimeException,
                                 "joynr.exceptions.JoynrMessageNotSentException")
MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::exceptions::JoynrDelayMessageException,
                                 joynr::exceptions::JoynrRuntimeException,
                                 "joynr.exceptions.JoynrDelayMessageException")
MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::exceptions::DiscoveryException,
                                 joynr::exceptions::JoynrRuntimeException,
                                 "joynr.exceptions.DiscoveryException")
MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::exceptions::ProviderRuntimeException,
                                 joynr::exceptions::JoynrRuntimeException,
                                 "joynr.exceptions.ProviderRuntimeException")
MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::exceptions::PublicationMissedException,
                                 joynr::exceptions::JoynrRuntimeException,
                                 "joynr.exceptions.PublicationMissedException")
MUESLI_REGISTER_POLYMORPHIC_TYPE(joynr::exceptions::ApplicationException,
                                 joynr::exceptions::JoynrException,
                                 "joynr.exceptions.ApplicationException")

#endif // EXCEPTIONS_H
