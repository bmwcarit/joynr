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
#ifndef JOYNR_URL_H
#define JOYNR_URL_H

#include "joynr/JoynrExport.h"

#include <cstdint>
#include <string>
#include <vector>

namespace joynr
{

/**
 * @brief The UrlQuery class
 */
class UrlQuery
{
public:
    /**
     * @brief UrlQuery
     */
    UrlQuery();
    /**
     * @brief addQueryItem
     * @param itemName
     * @param itemValue
     */
    void addQueryItem(const std::string& itemName, const std::string& itemValue);
    /**
     * @brief toString
     * @return
     */
    std::string toString() const;

private:
    std::vector<std::string> _queryItems;
};

/**
 * @brief Parses and encapsulates a URL
 */
class JOYNR_EXPORT Url
{
public:
    /**
     * @brief Url
     */
    Url();
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
        std::uint16_t port,
        const std::string& path);

    /**
     * @brief Create a URL from structured data
     * @param protocol The protocol
     * @param user The user to authenticate
     * @param password The authentication password
     * @param host The host
     * @param port The port number
     * @param path The path
     * @param query The query
     * @param fragment The fragment
     */
    Url(const std::string& _protocol,
        const std::string& _user,
        const std::string& _password,
        const std::string& _host,
        std::uint16_t _port,
        const std::string& _path,
        const std::string& _query,
        const std::string& _fragment);

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
    Url(Url&& other) noexcept = default;

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
    const std::string getProtocol() const;

    /**
     * @brief Get the user authentication part of the Url
     * @return The username
     */
    const std::string getUser() const;

    /**
     * @brief Get the passsword authentication part of the Url
     * @return The password
     */
    const std::string getPassword() const;

    /**
     * @brief Get the host part of the Url
     * @return The host
     */
    const std::string getHost() const;

    /**
     * @brief Get the port in the Url
     * @return The port number
     */
    std::uint16_t getPort() const;

    /**
     * @brief Get the path part of the Url
     * @return The path
     */
    const std::string getPath() const;

    /**
     * @brief setPath
     * @param path
     */
    void setPath(const std::string& path);

    /**
     * @brief Get the query of the Url
     * @return The query
     */
    const std::string getQuery() const;

    /**
     * @brief Set the query of the Url
     * @param The query
     */
    void setQuery(UrlQuery query);

    /**
     * @brief Get the fragment of the Url
     * @return The fragment
     */
    const std::string getFragment() const;

    /**
     * @brief Indicates if the URL is valid
     * @return true if the URL is valid, false otherwise
     */
    bool isValid() const;

    /**
     * @brief toString
     * @return
     */
    std::string toString() const;

private:
    std::string _protocol;
    std::string _user;
    std::string _password;
    std::string _host;
    std::uint16_t _port;
    std::string _path;
    std::string _query;
    std::string _fragment;
    bool _valid;
    bool _isIPv6HexAddress;

    void parseUrl(const std::string& text);
    std::uint16_t portFromProtocol(const std::string& proto);
    void validate();
};

} // namespace joynr
#endif // JOYNR_URL_H
