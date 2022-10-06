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

namespace joynr
{

Url::Url()
        : _protocol(),
          _user(),
          _password(),
          _host(),
          _port(),
          _path(),
          _query(),
          _fragment(),
          _valid(false)
{
}

Url::Url(const std::string& text)
        : _protocol(),
          _user(),
          _password(),
          _host(),
          _port(),
          _path(),
          _query(),
          _fragment(),
          _valid(false),
          _isIPv6HexAddress(false)
{
    parseUrl(text);
}

Url::Url(const std::string& protocol,
         const std::string& host,
         std::uint16_t port,
         const std::string& path)
        : _protocol(protocol),
          _user(),
          _password(),
          _host(host),
          _port(port),
          _path(path),
          _query(),
          _fragment(),
          _valid(false),
          _isIPv6HexAddress(false)
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
        : _protocol(protocol),
          _user(user),
          _password(password),
          _host(host),
          _port(port),
          _path(path),
          _query(query),
          _fragment(fragment),
          _valid(false),
          _isIPv6HexAddress(false)
{
    // Set valid to true if member variables are valid
    validate();
}

bool Url::operator==(const Url& other) const
{
    if (!(_valid && other._valid)) {
        return false;
    }

    return (_port == other._port && _protocol == other._protocol && _user == other._user &&
            _password == other._password && _host == other._host && _path == other._path &&
            _query == other._query && _fragment == other._fragment);
}

const std::string Url::getProtocol() const
{
    return _protocol;
}

const std::string Url::getUser() const
{
    return _user;
}

const std::string Url::getPassword() const
{
    return _password;
}

const std::string Url::getHost() const
{
    return _host;
}

std::uint16_t Url::getPort() const
{
    return _port;
}

const std::string Url::getPath() const
{
    return _path;
}

void Url::setPath(const std::string& path)
{
    this->_path = path;
    validate();
}

const std::string Url::getQuery() const
{
    return _query;
}

void Url::setQuery(UrlQuery query)
{
    this->_query = query.toString();
    validate();
}

const std::string Url::getFragment() const
{
    return _fragment;
}

bool Url::isValid() const
{
    return _valid;
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
        IPV6,
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
                _protocol += ch;
            }
            break;
        case State::SCHEME_SEP:
            if (ch != '/') {
                state = State::USER;
                branchStart = i;
                _user += ch;
            }
            break;
        case State::USER:
            if (ch == ':') {
                state = State::PASSWORD;
            } else if (ch == '/' || i == text.size() - 1) {
                // There was no auth - backtrack
                state = State::HOST;
                i = branchStart - 1;
                _user.clear();
            } else {
                _user += ch;
            }
            break;
        case State::PASSWORD:
            if (ch == '@') {
                state = State::HOST;
            } else if (ch == '/' || i == text.size() - 1) {
                // There was no auth - backtrack
                state = State::HOST;
                i = branchStart - 1;
                _user.clear();
                _password.clear();
            } else {
                _password += ch;
            }
            break;
        case State::HOST:
            if (ch == ':') {
                state = State::PORT;
            } else if (ch == '/') {
                state = State::PATH;
                _path += ch;
            } else if (ch == '[') {
                state = State::IPV6;
            } else {
                _host += ch;
            }
            break;
        case State::IPV6:
            if (ch == ']') {
                state = State::HOST;
            } else {
                _host += ch;
            }
            break;
        case State::PORT:
            if (ch == '/') {
                state = State::PATH;
                _path += ch;
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
                _path += ch;
            }
            break;
        case State::QUERY:
            if (ch == '#') {
                state = State::FRAGMENT;
            } else {
                _query += ch;
            }
            break;
        case State::FRAGMENT:
            _fragment += ch;
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
        _port = portFromProtocol(_protocol);
    } else {
        _port = std::stoi(portString);
    }

    // Set valid to true if the member variables appear correct
    validate();
}

std::string Url::toString() const
{
    std::stringstream stringBuilder;
    stringBuilder << _protocol << "://";
    if (!_user.empty()) {
        stringBuilder << _user;
        if (!_password.empty()) {
            stringBuilder << ":" << _password;
        }
        stringBuilder << "@";
    }
    stringBuilder << (_isIPv6HexAddress ? "[" : "") << _host << (_isIPv6HexAddress ? "]" : "");
    if (_port != 0) {
        stringBuilder << ":" << _port;
    }
    if (!_path.empty()) {
        stringBuilder << _path;
    }
    if (!_query.empty()) {
        stringBuilder << "?" << _query;
    }
    if (!_fragment.empty()) {
        stringBuilder << "#" << _fragment;
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
    if (_protocol.empty() || _host.empty()) {
        return;
    }
    // Post process the path
    if (_path.empty()) {
        _path = "/";
    }

    // only URI string contains [...] in case of IPv6 hexadecimal address
    if ((_host.find("[") != std::string::npos) || (_host.find("]") != std::string::npos)) {
        return;
    }

    if (_host.find(":") != std::string::npos) {
        _isIPv6HexAddress = true;
    }

    // Assume success
    _valid = true;
}

UrlQuery::UrlQuery() : _queryItems()
{
}

void UrlQuery::addQueryItem(const std::string& itemName, const std::string& itemValue)
{
    std::string queryItem = itemName + "=" + itemValue;
    _queryItems.push_back(queryItem);
}

std::string UrlQuery::toString() const
{
    if (_queryItems.empty()) {
        return "";
    }

    std::string result = boost::algorithm::join(_queryItems, "&");
    return result;
}

} // namespace joynr
