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
#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include "joynr/JoynrCommonExport.h"

#include <exception>
#include <QString>
#include <string>

namespace joynr {

class JOYNRCOMMON_EXPORT JoynrException : public std::exception {
public:
    JoynrException() throw();
    JoynrException(const std::string& what) throw();
    ~JoynrException() throw();
    virtual const char* what() const throw();
    virtual void setMessage(std::string what);

protected:
    std::string message;
};

class JOYNRCOMMON_EXPORT JoynrTimeOutException : public JoynrException {
public:
    JoynrTimeOutException() throw();
    JoynrTimeOutException(const std::string& what) throw();
};


class JOYNRCOMMON_EXPORT JoynrParseError : public JoynrException{
public:
    JoynrParseError(const std::string& what) throw();
};

class JOYNRCOMMON_EXPORT JoynrArbitrationException : public JoynrException {
public:
    JoynrArbitrationException(const std::string& what) throw();
};

class JOYNRCOMMON_EXPORT JoynrArbitrationFailedException : public JoynrException {
public:
    JoynrArbitrationFailedException(const std::string& what) throw();
};

class JOYNRCOMMON_EXPORT JoynrArbitrationTimeOutException : public JoynrException {
public:
    JoynrArbitrationTimeOutException(const std::string& what) throw();
};

class JOYNRCOMMON_EXPORT JoynrRuntimeException : public JoynrException {
public:
    JoynrRuntimeException(const std::string& what) throw();
};


} // namespace joynr
#endif //EXCEPTIONS_H
