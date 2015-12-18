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
#ifndef JOYNR_URL_H
#define JOYNR_URL_H

#include "joynr/JoynrCommonExport.h"

#include <string>

namespace joynr
{

/**
 * @brief Parses and encapsulates a URL
 */
class JOYNRCOMMON_EXPORT Url
{
public:
    /**
     * @brief Create a URL from text
     * @param text Text in a URL format e.g "http://www.name.com/some/path"
     */
    explicit Url(const std::string& text);

    /**
     * @brief Create a URL from structured data
     * @param protocol The protocol
     * @param host The host
     * @param port The port number
     * @param path The path
     */
    Url(const std::string& protocol,
        const std::string& host,
        uint16_t port,
        const std::string& path);

    /**
     * @brief Create a URL from structured data
     * @param protocol The protocol
     * @param user The user to authenticate
     * @param password The authentication password
     * @param host The host
     * @param port The port number
     * @param path The path
     */
    Url(const std::string& protocol,
        const std::string& user,
        const std::string& password,
        const std::string& host,
        uint16_t port,
        const std::string& path);

    /**
     * @brief Copy a Url
     * @param other The Url to copy
     */
    Url(const Url& other) = default;

    /**
     * @brief Copy a Url
     * @param other The Url to copy
     */
    Url& operator=(const Url& other) = default;

    /**
     * @brief Move constructor
     * @param other The Url to move
     */
    Url(Url&& other) = default;

    /**
     * @brief Indicate if two Urls are equal
     * @param other The Url to compare
     * @return true if equal, false otherwise
     */
    bool operator==(const Url& other) const;

    /**
     * @brief Get the protocol part of the Url, e.g https
     * @return The Protocol
     */
    const std::string& getProtocol() const;

    /**
     * @brief Get the user authentication part of the Url
     * @return The username
     */
    const std::string& getUser() const;

    /**
     * @brief Get the passsword authentication part of the Url
     * @return The password
     */
    const std::string& getPassword() const;

    /**
     * @brief Get the host part of the Url
     * @return The host
     */
    const std::string& getHost() const;

    /**
     * @brief Get the port in the Url
     * @return The port number
     */
    uint16_t getPort() const;

    /**
     * @brief Get the path part of the Url
     * @return The path
     */
    const std::string& getPath() const;

    /**
     * @brief Indicates if the URL is valid
     * @return true if the URL is valid, false otherwise
     */
    bool isValid() const;

private:
    std::string protocol;
    std::string user;
    std::string password;
    std::string host;
    uint16_t port;
    std::string path;
    bool valid;

    void parseUrl(const std::string& text);
    uint16_t portFromProtocol(const std::string& proto);
    void validate();
};

} // namespace joynr
#endif // JOYNR_URL_H
