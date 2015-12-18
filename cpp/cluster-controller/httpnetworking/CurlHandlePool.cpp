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
#include "cluster-controller/httpnetworking/CurlHandlePool.h"
#include "joynr/TypedClientMultiCache.h"

#ifdef WIN32
// qdatetime.h is included by one of the include files below
// This causes a compiler error with Qt 5.1 and VS2010
// https://qt-project.org/forums/viewthread/22133
#define NOMINMAX
#endif

#include <QMutableLinkedListIterator>
#include <QUrl>
#include <QThread>
#include <QVector>
#include <curl/curl.h>
#include <tuple>

namespace joynr
{

void* AlwaysNewCurlHandlePool::getHandle(const std::string& url)
{
    std::ignore = url;
    return curl_easy_init();
}

void AlwaysNewCurlHandlePool::returnHandle(void* handle)
{
    curl_easy_cleanup(handle);
}

void AlwaysNewCurlHandlePool::deleteHandle(void* handle)
{
    curl_easy_cleanup(handle);
}

void AlwaysNewCurlHandlePool::reset()
{
    // Do nothing
}

const int PerThreadCurlHandlePool::POOL_SIZE = 10;

PerThreadCurlHandlePool::PerThreadCurlHandlePool()
        : idleHandleMap(), handleOrderList(), outHandleMap(), mutex()
{
}

void* PerThreadCurlHandlePool::getHandle(const std::string& url)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::shared_ptr<PooledCurlHandle> pooledHandle;
    std::string host = extractHost(url);
    pooledHandle = takeOrCreateHandle(QThread::currentThreadId(), host);
    pooledHandle->setActiveHost(host);
    outHandleMap.insert(pooledHandle->getHandle(), pooledHandle);
    return pooledHandle->getHandle();
}

void PerThreadCurlHandlePool::returnHandle(void* handle)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::shared_ptr<PooledCurlHandle> pooledHandle = outHandleMap.take(handle);
    pooledHandle->clearHandle();
    idleHandleMap.insert(QThread::currentThreadId(), pooledHandle);
    // handles most recently used are prepended
    removeAll(handleOrderList, pooledHandle);
    handleOrderList.insert(handleOrderList.begin(), pooledHandle);

    if (!handleOrderList.empty() && handleOrderList.size() + outHandleMap.size() > POOL_SIZE) {
        // if the list of idle handles is too big, remove the last item of the ordered list
        std::shared_ptr<PooledCurlHandle> handle2remove = handleOrderList.back();
        handleOrderList.pop_back();
        for (Qt::HANDLE threadId : idleHandleMap.keys()) {
            int removed = idleHandleMap.remove(threadId, handle2remove);
            if (removed > 0) {
                break;
            }
        }
    }
}

void PerThreadCurlHandlePool::deleteHandle(void* handle)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::shared_ptr<PooledCurlHandle> pooledHandle = outHandleMap.take(handle);
    if (pooledHandle) {
        removeAll(handleOrderList, pooledHandle);
        idleHandleMap.remove(QThread::currentThreadId(), pooledHandle);
    }
}

void PerThreadCurlHandlePool::reset()
{
    std::lock_guard<std::mutex> lock(mutex);

    // Remove all idle handles
    handleOrderList.clear();
    idleHandleMap.clear();
}

std::string PerThreadCurlHandlePool::extractHost(const std::string& url)
{
    QUrl qurl(QString::fromStdString(url));
    return qurl.host().toStdString() + ":" + std::to_string(qurl.port());
}

std::shared_ptr<PooledCurlHandle> PerThreadCurlHandlePool::takeOrCreateHandle(
        const Qt::HANDLE& threadId,
        std::string host)
{
    if (idleHandleMap.contains(threadId)) {
        std::vector<std::shared_ptr<PooledCurlHandle>> handleList =
                idleHandleMap.values(threadId).toVector().toStdVector();
        auto i = handleList.cbegin();
        while (i != handleList.cend()) {
            std::shared_ptr<PooledCurlHandle> pooledHandle = *i;
            // prefer handles which have already been connected to the desired host address.
            // Reusing open connections has performance benefits
            if (pooledHandle->hasHost(host)) {
                idleHandleMap.remove(threadId, pooledHandle);
                return pooledHandle;
            }
        }
        return idleHandleMap.take(threadId);
    } else {
        return std::shared_ptr<PooledCurlHandle>(new PooledCurlHandle());
    }
}

const int PooledCurlHandle::CONNECTIONS_PER_HANDLE = 3;

PooledCurlHandle::PooledCurlHandle() : hosts(), handle(nullptr)
{
    handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_MAXCONNECTS, (long)CONNECTIONS_PER_HANDLE);
}

PooledCurlHandle::~PooledCurlHandle()
{
    curl_easy_cleanup(handle);
}

bool PooledCurlHandle::hasHost(const std::string& host) const
{
    return hosts.contains(host);
}

void* PooledCurlHandle::getHandle()
{
    return handle;
}

void PooledCurlHandle::setActiveHost(const std::string& host)
{
    QMutableLinkedListIterator<std::string> i(hosts);
    while (i.hasNext()) {
        const std::string& itHost = i.next();
        if (itHost == host) {
            i.remove();
            hosts.prepend(host);
            return;
        }
    }

    hosts.prepend(host);
    if (hosts.size() > CONNECTIONS_PER_HANDLE) {
        hosts.removeLast();
    }
}

void PooledCurlHandle::clearHandle()
{
    curl_easy_setopt(handle, CURLOPT_PROXY, "");
    curl_easy_setopt(handle, CURLOPT_POST, 0); // false
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, 0);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, 0);
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, NULL);
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, NULL);
}

// TODO make this configurable
const int SingleThreadCurlHandlePool::POOL_SIZE = 10;

void* SingleThreadCurlHandlePool::getHandle(const std::string& url)
{
    std::string host(extractHost(url));
    std::lock_guard<std::mutex> lock(mutex);

    std::shared_ptr<PooledCurlHandle> pooledHandle = takeOrCreateHandle(host);
    pooledHandle->setActiveHost(host);
    outHandleMap.insert(pooledHandle->getHandle(), pooledHandle);
    return pooledHandle->getHandle();
}

void SingleThreadCurlHandlePool::returnHandle(void* handle)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::shared_ptr<PooledCurlHandle> pooledHandle = outHandleMap.take(handle);
    pooledHandle->clearHandle();
    handleList.removeAll(pooledHandle);
    handleList.prepend(pooledHandle);
    if (handleList.size() + outHandleMap.size() > POOL_SIZE) {
        handleList.removeLast();
    }
}

void SingleThreadCurlHandlePool::deleteHandle(void* handle)
{
    std::lock_guard<std::mutex> lock(mutex);
    std::shared_ptr<PooledCurlHandle> pooledHandle = outHandleMap.take(handle);
    if (pooledHandle) {
        handleList.removeAll(pooledHandle);
    }
}

void SingleThreadCurlHandlePool::reset()
{
    std::lock_guard<std::mutex> lock(mutex);

    // Remove all idle handles
    handleList.clear();
}

std::string SingleThreadCurlHandlePool::extractHost(const std::string& url)
{
    QUrl qurl(QString::fromStdString(url));
    return qurl.host().toStdString() + ":" + std::to_string(qurl.port());
}

std::shared_ptr<PooledCurlHandle> SingleThreadCurlHandlePool::takeOrCreateHandle(
        const std::string& host)
{
    QMutableLinkedListIterator<std::shared_ptr<PooledCurlHandle>> i(handleList);
    while (i.hasNext()) {
        std::shared_ptr<PooledCurlHandle> handle = i.next();
        if (handle->hasHost(host)) {
            i.remove();
            return handle;
        }
    }

    if (!handleList.isEmpty()) {
        return handleList.takeLast();
    }

    return std::shared_ptr<PooledCurlHandle>(new PooledCurlHandle());
}

} // namespace joynr
