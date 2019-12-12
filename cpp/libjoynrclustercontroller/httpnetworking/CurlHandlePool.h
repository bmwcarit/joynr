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
#ifndef CURLHANDLEPOOL_H_
#define CURLHANDLEPOOL_H_

#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "joynr/JoynrClusterControllerExport.h"
#include "joynr/PrivateCopyAssign.h"

#include "HttpNetworking.h"

namespace joynr
{

/**
  * Simple implementation, that always creates a new handle and instantly frees it, when it is
 * returned.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT AlwaysNewCurlHandlePool : public ICurlHandlePool
{
public:
    void* getHandle(const std::string& url) override;
    void returnHandle(void* handle) override;
    void deleteHandle(void* handle) override;
    void reset() override;
};

/**
  * Encapsulates a pooled curl handle and the hosts it might have an open connection to.
  *
  * Hosts supplied to this class must have the syntax <hostName or ip>:<port>.
  * Hosts are compared using string comparison and not using DNS.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT PooledCurlHandle
{
public:
    PooledCurlHandle();
    ~PooledCurlHandle();

    /**
      * Checks if the handle might have an open connection to the specified host.
      *
      * This check is only a guess. We do not ask curl if the connection is open.
      * Instead the last CURLOPT_MAXCONNECTS host are internally saved in a linked list.
      */
    bool hasHost(const std::string& host) const;

    /**
      * Sets the host the pooled handle will connect to next.
      */
    void setActiveHost(const std::string& host);

    /**
      * Returns the curl handle.
      */
    void* getHandle();

    /**
      * Resets all options for the handle, so that it can be used again.
      * Should be used, when it is returned to the pool.
      */
    void clearHandle();

    /**
      * The number of connections that might be mantained by one curl handle at a time.
      * See CURLOPT_MAXCONNECTS in the curl documentation.
      */
    static const std::int32_t CONNECTIONS_PER_HANDLE;

private:
    DISALLOW_COPY_AND_ASSIGN(PooledCurlHandle);
    /**
      * Hosts in this linked list are ordered by last use. The most recently used host is at the
     * front.
      */
    std::list<std::string> hosts;
    void* handle;
    mutable std::mutex hostsMutex;
};

/**
  * The default implementation, that actually pools handles.
  *
  * !!! This implementation does NOT differentiate between multiple threads which ask for a handle.
  * Therefore it my only be used by a single thread. Use PerThreadCurlHandlePool if multiple threads
  * share the handle pool. !!!
  *
  * For pooled handles this implementation uses linked lists.
  * This might be not efficient enough for a large POOL_SIZE.
  *
  * The getHandle and returrnHandle methods are synchonized and therby thread-safe.
  * This can impeed performance in highly concurrent enviroments.
  */
class JOYNRCLUSTERCONTROLLER_EXPORT SingleThreadCurlHandlePool : public ICurlHandlePool
{
public:
    void* getHandle(const std::string& url) override;
    void returnHandle(void* handle) override;
    void deleteHandle(void* handle) override;
    void reset() override;

private:
    std::shared_ptr<PooledCurlHandle> takeOrCreateHandle(const std::string& host);

    /**
      * Handles in this linked list are ordered by last use. The most recently used handle is at the
     * front.
      */
    std::list<std::shared_ptr<PooledCurlHandle>> handleList;

    /**
      * Handles that are currently in use(rented using getHandle(url)).
      */
    std::unordered_map<void*, std::shared_ptr<PooledCurlHandle>> outHandleMap;

    /**
      * The number of handles that are internally pooled (out and on hold).
      * The pool will create an indefinite number of handles on request,
      * but will destroy them on return if the total number exceeds POOL_SIZE.
      *
      * Operating this thread pool when the total number of used handles
      * is often higher then POOL_SIZE will lead to decreased performance.
      */
    static const int POOL_SIZE;

    std::mutex mutex;
};

/**
  * Thread aware implementation of the curl handle pool: A handle will not be shared between
  *different threads.
  * This is essential as it would produce segmentation faults.
  * When getHandle is called, the id of the calling thread is checked. If the pool contains an idle
  *handle
  * which has been created for the same thread it will be reused. If there is no matching handle a
  *new one will be created.
  * If there are multiple idle handle in the pool, one which was already connected to the host will
  *be choosen.
  *
  */

class JOYNRCLUSTERCONTROLLER_EXPORT PerThreadCurlHandlePool : public ICurlHandlePool
{
public:
    PerThreadCurlHandlePool();

    /**
      * Handles returned by getHandle should NOT be forwarded to another thread (or even shared
     * between threads). Use the handle in the requesting thread only!
      */
    void* getHandle(const std::string& url) override;

    void returnHandle(void* handle) override;

    void deleteHandle(void* handle) override;

    void reset() override;

private:
    /**
      * If there already exists a handle for the current thread which is not in use at the moment it
     * is removed from the list and returned.
      * If no handle is available for the current thread, a new one is created.
      */
    std::shared_ptr<PooledCurlHandle> takeOrCreateHandle(const std::thread::id& threadId,
                                                         std::string host);

    /**
      * Handles in this Map are available for reuse. The keys are ThreadIds and values are
     * PooledCurlHandles.
      * At startup it is empty, new entries are added whenever a handle is returned with
     * returnHandle(void* handle)
      * and the size of this map plus the size of outHandleMap is smaller than POOL_SIZE
      */
    // TODO shouldn't the POOL_SIZE define the amount of idle handles?
    // key of this map: std::thread::id == the thread id the handle has been created for.
    std::unordered_multimap<std::thread::id, std::shared_ptr<PooledCurlHandle>> idleHandleMap;
    // handleOrderList is used to sort the curl handles according to their last use.
    // By deleting the last item of this list, the longest idle curl handle will be deleted.
    std::vector<std::shared_ptr<PooledCurlHandle>> handleOrderList;

    /**
      * Handles that are currently in use(rented using getHandle(url)).
      */
    std::unordered_map<void*, std::shared_ptr<PooledCurlHandle>> outHandleMap;

    static const int POOL_SIZE;
    std::mutex mutex;
};

} // namespace joynr
#endif // CURLHANDLEPOOL_H_
