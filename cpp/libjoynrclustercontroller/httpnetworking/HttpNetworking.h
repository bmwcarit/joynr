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
#ifndef HTTPNETWORKING_H
#define HTTPNETWORKING_H

#include <chrono>
#include <memory>
#include <string>

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class HttpResult;
class HttpRequestBuilder;

/**
  * This is an interface for http requests built by http builders.
  * The implementation guarantees, that the execute can be called multiple times.
  * The function is not thread safe but can be called from different threads
  * as long as it does not happen simultaneously.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT HttpRequest
{
public:
    /**
      * This method blocks until the execution finishes. It can block for a very long time.
      * To prevent this timeouts can be specified.
      */
    virtual HttpResult execute() = 0;

    /**
     * This method allows to immediately interrupt the execution of a HttpRequest.
     */
    virtual void interrupt() = 0;

    virtual ~HttpRequest() = default;
};

/**
  * Base interface for http builders. Contains methods, that are common to all http methods.
  */
template <class T>
class JOYNRCLUSTERCONTROLLER_EXPORT IHttpBuilder
{
public:
    /**
      * Creates a new HttpRequest using the builder. The builder must not be used after this method
     * was called.
      * The returned pointer must be deleted manually.
      */
    virtual HttpRequest* build() = 0;

    virtual ~IHttpBuilder() = default;
    /**
      * Sets a proxy for the built http request overriding the global proxy set in HttpNetworking.
      */
    virtual T* withProxy(const std::string& proxy) = 0;

    /**
      * Enables HTTP logging if available. The method of logging is dependent on the HTTP library
      * being used.
      */
    virtual T* withDebug() = 0;

    /**
     * Adds a certificate authority certificate file so that the server can be authenticated:
     */
    virtual T* withCertificateAuthority(const std::string& caFile) = 0;

    /**
     * Adds a client certificate so that the server can authenticate this client
     */
    virtual T* withClientCertificate(const std::string& certificateFile) = 0;

    /**
     * Enables the client certificate to be unlocked with a password
     */
    virtual T* withClientCertificatePassword(const std::string& password) = 0;

    /**
      * Adds an http header. Only ASCII characters are allowed.
      */
    virtual T* addHeader(const std::string& name, const std::string& value) = 0;
    virtual T* withConnectTimeout(std::chrono::milliseconds timeout) = 0;
    virtual T* withTimeout(std::chrono::milliseconds timeout) = 0;
};

/**
  * Interface for http get builders. Contains methods, that are specific to http GET.
  * Derives all the common methods from HttpBuilder.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT IHttpGetBuilder : public IHttpBuilder<IHttpGetBuilder>
{
public:
    /**
     * Accept responses from the server that have been gzipped
     */
    virtual IHttpGetBuilder* acceptGzip() = 0;
};

/**
  * Interface for http delete builders. Contains methods, that are specific to http DELETE.
  * Derives all the common methods from HttpBuilder.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT IHttpDeleteBuilder : public IHttpBuilder<IHttpDeleteBuilder>
{
};

/**
  * Interface for http post builders. Contains methods, that are specific to http POST.
  * Derives all the common methods from HttpBuilder.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT IHttpPostBuilder : public IHttpBuilder<IHttpPostBuilder>
{
public:
    /**
      * Sets the content type. Only ASCII characters are allowed.
      */
    virtual IHttpPostBuilder* withContentType(const std::string& contentType) = 0;

    /**
      * Tells to post the data supplied. No copy of the data is internally created.
      * The caller must ensure that the pointer stays valid until the built HttpRequest is no longer
     * used.
      */
    virtual IHttpPostBuilder* postContent(const std::string& data) = 0;
    ~IHttpPostBuilder() override = default;
};

/**
  * Encapsulates the management of curl handles. Used internally by HttpNetworking.
  *
  * The user has to return all handles aquiered by getHandle(...) using returnHandle(...)
  */
class JOYNRCLUSTERCONTROLLER_EXPORT ICurlHandlePool
{
public:
    virtual void* getHandle(const std::string& url) = 0;
    virtual void returnHandle(void* handle) = 0;

    /**
     * Delete handle e.g if an error occurs
     * @param handle The handle to delete
     */
    virtual void deleteHandle(void* handle) = 0;

    /**
     * Remove all handles
     */
    virtual void reset() = 0;

    virtual ~ICurlHandlePool() = default;
};

/**
  * Encapsulates the http networking for Joynr.
  * Offers methods to set the global proxy settings and to create builders to build http requests.
  *
  * Builders returned by this class can be used to build a request only once. Calling the method
  *more than once will result in an exception beeing thrown.
  * Pointers to created builders must be deleted when no longer used.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT HttpNetworking
{
public:
    ~HttpNetworking() = default;

    static std::shared_ptr<HttpNetworking> getInstance();
    IHttpGetBuilder* createHttpGetBuilder(const std::string& url);
    IHttpDeleteBuilder* createHttpDeleteBuilder(const std::string& url);
    IHttpPostBuilder* createHttpPostBuilder(const std::string& url);

    /**
      * Used internally
      */
    std::shared_ptr<ICurlHandlePool> getCurlHandlePool() const;

    /**
      * Sets the global proxy to the specified string in the format "host:port".
      * If the string is empty the proxy is unset.
      * This setting only affects http builders created after the method was called.
      */
    void setGlobalProxy(const std::string& proxy);

    /**
     * Sets the HTTP connect timeout
     */
    void setConnectTimeout(std::chrono::milliseconds connectTimeout);

    /**
      * Enables HTTP logging if available. The method of logging is dependent on the HTTP library
      * being used.
      */
    void setHTTPDebugOn();

    /**
      * Sets the certificate authority that will be used to authenticate the server for HTTPS
     * requests
      */
    void setCertificateAuthority(const std::string& certificateAuthority);

    /**
      * Sets the client certificate that the server will use to authenticate the client during HTTPS
     * requests
      */
    void setClientCertificate(const std::string& clientCertificate);

    /**
      * Enables a password protected client certificate to be unlocked
      */
    void setClientCertificatePassword(const std::string& clientCertificatePassword);

private:
    DISALLOW_COPY_AND_ASSIGN(HttpNetworking);
    HttpNetworking();
    HttpRequestBuilder* createRequestBuilder(const std::string& url);
    std::shared_ptr<ICurlHandlePool> _curlHandlePool;

    std::string _proxy;
    std::chrono::milliseconds _connectTimeout;
    std::string _certificateAuthority;
    std::string _clientCertificate;
    std::string _clientCertificatePassword;
    bool _httpDebug;
};

} // namespace joynr
#endif // HTTPNETWORKING_H
