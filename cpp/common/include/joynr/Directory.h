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
#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <string>
#include <mutex>
#include <unordered_map>
#include <memory>

#include "joynr/PrivateCopyAssign.h"
#include "joynr/SingleThreadedDelayedScheduler.h"
#include "joynr/Runnable.h"
#include "joynr/ITimeoutListener.h"
#include "joynr/Logger.h"
#include "joynr/IReplyCaller.h"
#include "joynr/serializer/Serializer.h"

namespace joynr
{

/**
  * The directory.h offers the interface of a Map. However, in contrast to a Map,
  * one can choose to use two different add methods. The first add/remove behave as expected by a
  *Map.
  * The second option is to specify a time to live for entries when adding them (in milli-secondes).
  *The
  * entry will be removed automatically after this time. The methods are thread-safe.
  *
  * This template can be used on libJoynr and ClusterController sides:
  *     MessagingEndpointDirectory,           CC
  *     ParticipantDirectory,                 CC
  *     \<Middleware\>RequestCallerDirectory, libjoynr
  *     ReplyCallerDirectory,                 libjoynr
  */
template <typename Key, typename T>
class Directory;

template <typename Key, typename T>
class RemoverRunnable : public Runnable
{
public:
    RemoverRunnable(const Key& keyId, Directory<Key, T>* directory)
            : Runnable(true), keyId(keyId), directory(directory)
    {
    }

    void shutdown() override
    {
    }

    void run() override
    {
        runImpl(std::is_same<T, IReplyCaller>{});
    }

private:
    void runImpl(std::false_type)
    {
        std::shared_ptr<T> val = directory->lookup(keyId);
        if (val) {
            directory->remove(keyId);
        }
    }

    /*
     * some objects that are put into the Directory need to be notified when the object is deleted
     * (e.g. the IReplyCaller will wait for a timeout from TTL). The RemoverRunnable therefore has a
     * partial specialisation for IReplyCallers and will call timeOut in this case.
     * This is also the reason why the IReplyCaller has to be in Common, instead of libjoynr/Proxy
     */
    void runImpl(std::true_type)
    {
        std::shared_ptr<IReplyCaller> value = directory->lookup(keyId);
        if (value) {
            value->timeOut();
            directory->remove(keyId);
        }
    }

    DISALLOW_COPY_AND_ASSIGN(RemoverRunnable);
    Key keyId;
    Directory<Key, T>* directory;
};

template <typename Key, typename T>
class Directory
{
public:
    Directory() = default;

    explicit Directory(const std::string& directoryName)
            : callbackMap(), mutex(), callBackRemoverScheduler("DirRemover")
    {
        std::ignore = directoryName;
    }

    ~Directory()
    {
        callBackRemoverScheduler.shutdown();
        JOYNR_LOG_TRACE(logger, "destructor: number of entries = {}", callbackMap.size());
    }

    /*
     * Returns the element with the given keyId. In case the element could not be found nullptr is
     * returned.
     */
    std::shared_ptr<T> lookup(const Key& keyId)
    {
        std::lock_guard<std::mutex> lock(mutex);
        auto found = callbackMap.find(keyId);
        if (found == callbackMap.cend()) {
            return nullptr;
        }
        return found->second;
    }

    /*
     * Returns true if an element with the given keyId could be found. False otherwise.
     */
    bool contains(const Key& keyId)
    {
        std::lock_guard<std::mutex> lock(mutex);
        return callbackMap.find(keyId) != callbackMap.cend();
    }

    /*
     * Adds an element and keeps it until actively removed (using the 'remove' method)
     */
    void add(const Key& keyId, std::shared_ptr<T> value)
    {
        std::lock_guard<std::mutex> lock(mutex);
        callbackMap[keyId] = std::move(value);
    }

    /*
     * Adds an element and removes it automatically after ttl_ms milliseconds have past.
     */
    void add(const Key& keyId, std::shared_ptr<T> value, std::int64_t ttl_ms)
    {
        // Insert the value
        {
            std::lock_guard<std::mutex> lock(mutex);
            callbackMap[keyId] = std::move(value);
        }

        // make a removerRunnable and shedule it to remove the entry after ttl!
        auto* removerRunnable = new RemoverRunnable<Key, T>(keyId, this);
        callBackRemoverScheduler.schedule(removerRunnable, std::chrono::milliseconds(ttl_ms));
    }

    /*
     * Remove element with key == keyID
     */
    void remove(const Key& keyId)
    {
        std::lock_guard<std::mutex> lock(mutex);
        callbackMap.erase(keyId);
    }

    template <typename Archive>
    void serialize(Archive& archive)
    {
        archive(MUESLI_NVP(callbackMap));
    }

protected:
    std::unordered_map<Key, std::shared_ptr<T>> callbackMap;
    ADD_LOGGER(Directory);

private:
    DISALLOW_COPY_AND_ASSIGN(Directory);
    std::mutex mutex;
    SingleThreadedDelayedScheduler callBackRemoverScheduler;
};

template <typename Key, typename T>
INIT_LOGGER(SINGLE_MACRO_ARG(Directory<Key, T>));
} // namespace joynr

#endif // DIRECTORY_H
