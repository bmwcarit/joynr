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
#ifndef ICALLBACK_H
#define ICALLBACK_H

#include "joynr/RequestStatus.h"

namespace joynr {

template <class T>

/**
 * @brief ICallback<T> interface is required for making asynchronous calls to a proxy.
 * An application should implement this, typing the interface according to the
 * expected type that should be returned as a result of the callback.
 * Note that "void" is also a valid type to signify no return value.
 *
 * T is the return type that is expected from the callback.
 */
class ICallback {
public:

    virtual ~ICallback () {}

    /**
     * @brief If an error occurs during the processing of the asychronous request,
     * this method will be called to notify the application.
     *
     * @param status Contains details about the failure.
     */
    virtual void onFailure(const RequestStatus status) = 0;

    /**
     * @brief This will be called after the async request has been successfully
     * executed and a return value is received.
     *
     * @param status Details about the request processed.
     * @param result The return type from the asyhchronous request.
     */
    virtual void onSuccess(const RequestStatus status, T result) = 0;
};

template <>
/**
 * @brief This is a specialisation of the ICallback<T> interface for a void type.
 */
class ICallback<void> {
public:
    virtual ~ICallback () {}


    /**
     * @brief This is called when a failure occurs for the request.
     *
     * @param status Containing an explanation why it failed.
     */
    virtual void onFailure(const RequestStatus status) = 0;


    /**
     * @brief This is called when the async method succeeds.
     * Note that the method differs to the ICallback<T> interface in that there
     * is no returned type.
     *
     * @param status Details about the request processed.
     */
    virtual void onSuccess(const RequestStatus status) = 0;
};


} // namespace joynr
#endif //ICALLBACK_H
