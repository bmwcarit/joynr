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
#ifndef HTTPNETWORKING_H_
#define HTTPNETWORKING_H_
#include "joynr/PrivateCopyAssign.h"

#include "joynr/JoynrClusterControllerExport.h"
#include <QString>

namespace joynr {

class HttpResult;
class HttpRequestBuilder;

/**
  * This is an interface for http requests built by http builders.
  * The implementation guarantees, that the execute can be called multiple times.
  * The function is not thread safe but can be called from different threads
  * as long as it does not happen simultaneously.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT HttpRequest {
public:
    /**
      * This method blocks until the execution finishes. It can block for a very long time.
      * To prevent this timeouts can be specified.
      */
    virtual HttpResult execute() = 0;

    virtual ~HttpRequest() {}
};

/**
  * Base interface for http builders. Contains methods, that are common to all http methods.
  */
template<class T> class JOYNRCLUSTERCONTROLLER_EXPORT IHttpBuilder {
public:

    /**
      * Creates a new HttpRequest using the builder. The builder must not be used after this method was called.
      * The returned pointer must be deleted manually.
      */
    virtual HttpRequest* build() = 0;

    virtual ~IHttpBuilder() {};
    /**
      * Sets a proxy for the built http request overriding the global proxy set in HttpNetworking.
      */
    virtual T* withProxy(const QString& proxy) = 0;

    /**
      * Enables HTTP logging if available. The method of logging is dependent on the HTTP library
      * being used.
      */
    virtual T* withDebug() = 0;

    /**
      * Adds an http header. Only ASCII characters are allowed.
      */
    virtual T* addHeader(const QString& name, const QString& value) = 0;
    virtual T* withTimeout_ms(long timeout_ms) = 0;
};

/**
  * Interface for http get builders. Contains methods, that are specific to http GET.
  * Derives all the common methods from HttpBuilder.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT IHttpGetBuilder : public IHttpBuilder<IHttpGetBuilder> {
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
class JOYNRCLUSTERCONTROLLER_EXPORT IHttpDeleteBuilder : public IHttpBuilder<IHttpDeleteBuilder> {
};

/**
  * Interface for http post builders. Contains methods, that are specific to http POST.
  * Derives all the common methods from HttpBuilder.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT IHttpPostBuilder : public IHttpBuilder<IHttpPostBuilder> {
public:
    /**
      * Sets the content type. Only ASCII characters are allowed.
      */
    virtual IHttpPostBuilder* withContentType(const QString& contentType) = 0;

    /**
      * Tells to post the data supplied. No copy of the data is internally created.
      * The caller must ensure that the pointer stays valid until the built HttpRequest is no longer used.
      */
    virtual IHttpPostBuilder* postContent(const QByteArray& data) = 0;
    virtual ~IHttpPostBuilder();
};


/**
  * Encapsulates the management of curl handles. Used internally by HttpNetworking.
  *
  * The user has to return all handles aquiered by getHandle(...) using returnHandle(...)
  */
class JOYNRCLUSTERCONTROLLER_EXPORT ICurlHandlePool {
public:
    virtual void* getHandle(const QString& url) = 0;
    virtual void returnHandle(void* handle) = 0;
    virtual ~ICurlHandlePool();
};

/**
  * Encapsulates the http networking for Joynr.
  * Offers methods to set the global proxy settings and to create builders to build http requests.
  *
  * Builders returned by this class can be used to build a request only once. Calling the method more than once will result in an exception beeing thrown.
  * Pointers to created builders must be deleted when no longer used.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT HttpNetworking {
public:
    static HttpNetworking* getInstance();

    IHttpGetBuilder* createHttpGetBuilder(const QString& url);
    IHttpDeleteBuilder* createHttpDeleteBuilder(const QString& url);
    IHttpPostBuilder* createHttpPostBuilder(const QString& url);

    /**
      * Used internally
      */
    ICurlHandlePool* getCurlHandlePool();

    /**
      * Sets the global proxy to the specified string in the format "host:port".
      * If the string is empty the proxy is unset.
      * This setting only affects http builders created after the method was called.
      */
    void setGlobalProxy(const QString& proxy);

    /**
      * Enables HTTP logging if available. The method of logging is dependent on the HTTP library
      * being used.
      */
    void setHTTPDebugOn();

private:
    DISALLOW_COPY_AND_ASSIGN(HttpNetworking);
    HttpNetworking();
    HttpRequestBuilder* createRequestBuilder(const QString& url);
    static HttpNetworking* httpNetworking;
    ICurlHandlePool* curlHandlePool;

    QString proxy;
    bool httpDebug;
};


} // namespace joynr
#endif //HTTPNETWORKING_H_
