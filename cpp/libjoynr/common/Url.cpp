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

#include "joynr/Url.h"
#include <sstream>
#include <boost/algorithm/string/join.hpp>
#include <cstddef>

namespace joynr
{

Url::Url()
        : protocol(), user(), password(), host(), port(), path(), query(), fragment(), valid(false)
{
}

Url::Url(const std::string& text)
        : protocol(), user(), password(), host(), port(), path(), query(), fragment(), valid(false)
{
    parseUrl(text);
}

Url::Url(const std::string& protocol,
         const std::string& host,
         std::uint16_t port,
         const std::string& path)
        : protocol(protocol),
          user(),
          password(),
          host(host),
          port(port),
          path(path),
          query(),
          fragment(),
          valid(false)
{
    // Set valid to true if member variables are valid
    validate();
}

Url::Url(const std::string& protocol,
         const std::string& user,
         const std::string& password,
         const std::string& host,
         std::uint16_t port,
         const std::string& path,
         const std::string& query,
         const std::string& fragment)
        : protocol(protocol),
          user(user),
          password(password),
          host(host),
          port(port),
          path(path),
          query(query),
          fragment(fragment),
          valid(false)
{
    // Set valid to true if member variables are valid
    validate();
}

bool Url::operator==(const Url& other) const
{
    if (!(valid && other.valid)) {
        return false;
    }

    return (port == other.port && protocol == other.protocol && user == other.user &&
            password == other.password && host == other.host && path == other.path &&
            query == other.query && fragment == other.fragment);
}

const std::string& Url::getProtocol() const
{
    return protocol;
}

const std::string& Url::getUser() const
{
    return user;
}

const std::string& Url::getPassword() const
{
    return password;
}

const std::string& Url::getHost() const
{
    return host;
}

std::uint16_t Url::getPort() const
{
    return port;
}

const std::string& Url::getPath() const
{
    return path;
}

void Url::setPath(const std::string& path)
{
    this->path = path;
    validate();
}

const std::string& Url::getQuery() const
{
    return query;
}

void Url::setQuery(UrlQuery query)
{
    this->query = query.toString();
    validate();
}

const std::string& Url::getFragment() const
{
    return fragment;
}

bool Url::isValid() const
{
    return valid;
}

void Url::parseUrl(const std::string& text)
{
    // scheme:[//[user:password@]host[:port]][/]path[?query][#fragment]
    enum class State {
        SCHEME,
        SCHEME_SEP,
        USER,
        PASSWORD,
        HOST,
        PORT,
        PATH,
        QUERY,
        FRAGMENT,
        TERMINATE
    };
    State state = State::SCHEME;

    std::string portString;

    std::size_t branchStart = 0; // used for backtracking

    // Loop through the text
    for (std::size_t i = 0; i < text.size() && state != State::TERMINATE; i++) {
        char ch = text[i];

        switch (state) {
        case State::SCHEME:
            if (ch == ':') {
                state = State::SCHEME_SEP;
            } else {
                protocol += ch;
            }
            break;
        case State::SCHEME_SEP:
            if (ch != '/') {
                state = State::USER;
                branchStart = i;
                user += ch;
            }
            break;
        case State::USER:
            if (ch == ':') {
                state = State::PASSWORD;
            } else if (ch == '/' || i == text.size() - 1) {
                // There was no auth - backtrack
                state = State::HOST;
                i = branchStart - 1;
                user.clear();
            } else {
                user += ch;
            }
            break;
        case State::PASSWORD:
            if (ch == '@') {
                state = State::HOST;
            } else if (ch == '/' || i == text.size() - 1) {
                // There was no auth - backtrack
                state = State::HOST;
                i = branchStart - 1;
                user.clear();
                password.clear();
            } else {
                password += ch;
            }
            break;
        case State::HOST:
            if (ch == ':') {
                state = State::PORT;
            } else if (ch == '/') {
                state = State::PATH;
                path += ch;
            } else {
                host += ch;
            }
            break;
        case State::PORT:
            if (ch == '/') {
                state = State::PATH;
                path += ch;
            } else {
                portString += ch;
            }
            break;
        case State::PATH:
            if (ch == '?') {
                state = State::QUERY;
            } else if (ch == '#') {
                state = State::FRAGMENT;
            } else {
                path += ch;
            }
            break;
        case State::QUERY:
            if (ch == '#') {
                state = State::FRAGMENT;
            } else {
                query += ch;
            }
            break;
        case State::FRAGMENT:
            fragment += ch;
            if (i == text.size()) {
                state = State::TERMINATE;
            }
            break;
        case State::TERMINATE:
            // Ignored
            break;
        }
    }

    // Post process the port
    if (portString.empty()) {
        port = portFromProtocol(protocol);
    } else {
        port = std::stoi(portString);
    }

    // Set valid to true if the member variables appear correct
    validate();
}

std::string Url::toString()
{
    std::stringstream stringBuilder;
    stringBuilder << protocol << "://";
    if (!user.empty()) {
        stringBuilder << user;
        if (!password.empty()) {
            stringBuilder << ":" << password;
        }
        stringBuilder << "@";
    }
    stringBuilder << host;
    if (port != 0) {
        stringBuilder << ":" << port;
    }
    if (!path.empty()) {
        stringBuilder << path;
    }
    if (!query.empty()) {
        stringBuilder << "?" << query;
    }
    if (!fragment.empty()) {
        stringBuilder << "#" << fragment;
    }

    return stringBuilder.str();
}

std::uint16_t Url::portFromProtocol(const std::string& proto)
{
    if (proto == "http") {
        return 80;
    } else if (proto == "https") {
        return 443;
    } else if (proto == "ws") {
        return 80;
    } else if (proto == "wss") {
        return 443;
    } else {
        return 80;
    }
}

void Url::validate()
{
    // Check - valid will remain false on error
    if (protocol.empty() || host.empty()) {
        return;
    }
    // Post process the path
    if (path.empty()) {
        path = "/";
    }

    // Assume success
    valid = true;
}

UrlQuery::UrlQuery() : queryItems()
{
}

void UrlQuery::addQueryItem(const std::string& itemName, const std::string& itemValue)
{
    std::string queryItem = itemName + "=" + itemValue;
    queryItems.push_back(queryItem);
}

std::string UrlQuery::toString()
{
    if (queryItems.empty())
        return "";

    std::string result = boost::algorithm::join(queryItems, "&");
    return result;
}

} // namespace joynr
