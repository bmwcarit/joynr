/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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
#include "CurlHandlePool.h"

#include <algorithm>
#include <tuple>

#include <curl/curl.h>

#include "joynr/Url.h"
#include "joynr/Util.h"

namespace joynr
{

std::string extractHost(const std::string& urlString)
{
    joynr::Url url(urlString);
    return url.getHost() + ":" + std::to_string(url.getPort());
}

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
    pooledHandle = takeOrCreateHandle(std::this_thread::get_id(), host);
    pooledHandle->setActiveHost(host);
    outHandleMap.insert({pooledHandle->getHandle(), pooledHandle});
    return pooledHandle->getHandle();
}

void PerThreadCurlHandlePool::returnHandle(void* handle)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = outHandleMap.find(handle);
    if (it == outHandleMap.cend()) {
        return;
    }
    std::shared_ptr<PooledCurlHandle> pooledHandle = it->second;
    outHandleMap.erase(it);
    pooledHandle->clearHandle();
    idleHandleMap.insert({std::this_thread::get_id(), pooledHandle});
    // handles most recently used are prepended
    util::removeAll(handleOrderList, pooledHandle);
    handleOrderList.insert(handleOrderList.begin(), pooledHandle);

    if (!handleOrderList.empty() && handleOrderList.size() + outHandleMap.size() > POOL_SIZE) {
        // if the list of idle handles is too big, remove the last item of the ordered list
        std::shared_ptr<PooledCurlHandle> handle2remove = handleOrderList.back();
        handleOrderList.pop_back();

        auto keys = util::getKeyVectorForMap(idleHandleMap);
        for (const std::thread::id& threadId : keys) {
            std::size_t removed =
                    util::removeAllPairsFromMultiMap(idleHandleMap, {threadId, handle2remove});
            if (removed > 0) {
                break;
            }
        }
    }
}

void PerThreadCurlHandlePool::deleteHandle(void* handle)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = outHandleMap.find(handle);
    if (it != outHandleMap.cend()) {
        util::removeAll(handleOrderList, it->second);
        util::removeAllPairsFromMultiMap(idleHandleMap, {std::this_thread::get_id(), it->second});
    }
}

void PerThreadCurlHandlePool::reset()
{
    std::lock_guard<std::mutex> lock(mutex);

    // Remove all idle handles
    handleOrderList.clear();
    idleHandleMap.clear();
}

std::shared_ptr<PooledCurlHandle> PerThreadCurlHandlePool::takeOrCreateHandle(
        const std::thread::id& threadId,
        std::string host)
{
    if (idleHandleMap.find(threadId) != idleHandleMap.cend()) {
        auto range = idleHandleMap.equal_range(threadId);
        for (auto it = range.first; it != range.second; ++it) {
            std::shared_ptr<PooledCurlHandle> pooledHandle = it->second;
            // prefer handles which have already been connected to the desired host address.
            // Reusing open connections has performance benefits
            if (pooledHandle->hasHost(host)) {
                idleHandleMap.erase(it);
                return pooledHandle;
            }
        }
        std::shared_ptr<PooledCurlHandle> pooledHandle = range.first->second;
        idleHandleMap.erase(range.first);
        return pooledHandle;
    } else {
        return std::make_shared<PooledCurlHandle>();
    }
}

const std::int32_t PooledCurlHandle::CONNECTIONS_PER_HANDLE = 3;

PooledCurlHandle::PooledCurlHandle() : hosts(), handle(nullptr), hostsMutex()
{
    handle = curl_easy_init();
    curl_easy_setopt(handle, CURLOPT_MAXCONNECTS, CONNECTIONS_PER_HANDLE);
}

PooledCurlHandle::~PooledCurlHandle()
{
    curl_easy_cleanup(handle);
}

bool PooledCurlHandle::hasHost(const std::string& host) const
{
    std::lock_guard<std::mutex> hostsLocker(hostsMutex);
    return (std::find(hosts.cbegin(), hosts.cend(), host) != hosts.cend());
}

void* PooledCurlHandle::getHandle()
{
    return handle;
}

void PooledCurlHandle::setActiveHost(const std::string& host)
{
    std::lock_guard<std::mutex> hostsLocker(hostsMutex);

    for (auto it = hosts.begin(); it != hosts.end(); ++it) {
        if (host == *it) {
            hosts.erase(it);
            hosts.push_front(host);
            return;
        }
    }

    hosts.push_front(host);
    if (hosts.size() > CONNECTIONS_PER_HANDLE) {
        hosts.pop_back();
    }
}

void PooledCurlHandle::clearHandle()
{
    curl_easy_setopt(handle, CURLOPT_PROXY, "");
    curl_easy_setopt(handle, CURLOPT_POST, 0); // false
    curl_easy_setopt(handle, CURLOPT_TIMEOUT_MS, 0);
    curl_easy_setopt(handle, CURLOPT_CONNECTTIMEOUT_MS, 0);
    curl_easy_setopt(handle, CURLOPT_HTTPHEADER, nullptr);
    curl_easy_setopt(handle, CURLOPT_CUSTOMREQUEST, nullptr);
}

// TODO make this configurable
const int SingleThreadCurlHandlePool::POOL_SIZE = 10;

void* SingleThreadCurlHandlePool::getHandle(const std::string& url)
{
    std::string host = extractHost(url);
    std::lock_guard<std::mutex> lock(mutex);

    std::shared_ptr<PooledCurlHandle> pooledHandle = takeOrCreateHandle(host);
    pooledHandle->setActiveHost(host);
    outHandleMap.insert({pooledHandle->getHandle(), pooledHandle});
    return pooledHandle->getHandle();
}

void SingleThreadCurlHandlePool::returnHandle(void* handle)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = outHandleMap.find(handle);
    if (it != outHandleMap.cend()) {
        std::shared_ptr<PooledCurlHandle> pooledHandle = it->second;
        outHandleMap.erase(it);
        pooledHandle->clearHandle();
        handleList.remove(pooledHandle);
        handleList.push_front(pooledHandle);
        if (handleList.size() + outHandleMap.size() > POOL_SIZE) {
            handleList.pop_back();
        }
    }
}

void SingleThreadCurlHandlePool::deleteHandle(void* handle)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto it = outHandleMap.find(handle);
    if (it != outHandleMap.cend()) {
        handleList.remove(it->second);
        outHandleMap.erase(it);
    }
}

void SingleThreadCurlHandlePool::reset()
{
    std::lock_guard<std::mutex> lock(mutex);

    // Remove all idle handles
    handleList.clear();
}

std::shared_ptr<PooledCurlHandle> SingleThreadCurlHandlePool::takeOrCreateHandle(
        const std::string& host)
{
    for (auto it = handleList.begin(); it != handleList.end(); ++it) {
        std::shared_ptr<PooledCurlHandle> handle = *it;
        if (handle->hasHost(host)) {
            handleList.erase(it);
            return handle;
        }
    }

    if (!handleList.empty()) {
        auto handle = handleList.back();
        handleList.pop_back();
        return handle;
    }

    return std::make_shared<PooledCurlHandle>();
}

} // namespace joynr
