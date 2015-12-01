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
#include "joynr/PrivateCopyAssign.h"

#include "joynr/DelayedScheduler.h"
#include "joynr/ITimeoutListener.h"
#include "joynr/joynrlogging.h"
#include <typeinfo>
#include "joynr/IReplyCaller.h"

#include <QString>
#include <string>
#include <functional>
#include <QtGlobal>
#include <QMutex>
#include <QHash>

#include <memory>

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
  *     MessagingEndpointDirectory,         CC
  *     ParticipantDirectory,               CC
  *     <Middleware>RequestCallerDirectory, libjoynr
  *     ReplyCallerDirectory,               libjoynr
  */

template <typename Key, typename T>
class IDirectory
{
public:
    virtual ~IDirectory()
    {
    }
    virtual std::shared_ptr<T> lookup(const Key& keyId) = 0;
    virtual bool contains(const Key& keyId) = 0;

    virtual void add(const Key& keyId, T* value) = 0;
    virtual void add(const Key& keyId, std::shared_ptr<T> value) = 0;

    virtual void add(const Key& keyId, T* value, qint64 ttl_ms) = 0;
    virtual void add(const Key& keyId, std::shared_ptr<T> value, qint64 ttl_ms) = 0;
    virtual void remove(const Key& keyId) = 0;
};

template <typename Key, typename T>
class Directory : public IDirectory<Key, T>
{

public:
    virtual ~Directory();
    Directory(const std::string& directoryName);
    std::shared_ptr<T> lookup(const Key& keyId);
    bool contains(const Key& keyId);
    /*
     * Adds an element and keeps it until actively removed (using the 'remove' method)
     */
    void add(const Key& keyId, T* value);
    void add(const Key& keyId, std::shared_ptr<T> value);
    /*
     * Adds an element and removes it automatically after ttl_ms milliseconds have past.
     */
    void add(const Key& keyId, T* value, qint64 ttl_ms);
    void add(const Key& keyId, std::shared_ptr<T> value, qint64 ttl_ms);
    void remove(const Key& keyId);

private:
    DISALLOW_COPY_AND_ASSIGN(Directory);
    QHash<Key, std::shared_ptr<T>> callbackMap;
    QMutex mutex;
    SingleThreadedDelayedScheduler callBackRemoverScheduler;
    static joynr_logging::Logger* logger;
};

template <typename Key, typename T>
class RemoverRunnable : public QRunnable
{
public:
    RemoverRunnable(const Key& keyId, Directory<Key, T>* directory);
    void run();

private:
    DISALLOW_COPY_AND_ASSIGN(RemoverRunnable);
    Key keyId;
    Directory<Key, T>* directory;
    static joynr_logging::Logger* logger;
};

template <typename Key, typename T>
joynr_logging::Logger* RemoverRunnable<Key, T>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "RemoverRunnable");

template <typename Key, typename T>
joynr_logging::Logger* Directory<Key, T>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "Directory");

template <typename Key, typename T>
Directory<Key, T>::~Directory()
{
    LOG_TRACE(logger,
              QString("destructor: number of entries = ") + QString::number(callbackMap.size()));
}

template <typename Key, typename T>
Directory<Key, T>::Directory(const std::string& directoryName)
        : callbackMap(),
          mutex(),
          callBackRemoverScheduler(QString::fromStdString(directoryName) +
                                   QString("-CleanUpScheduler"))
{
}

template <typename Key, typename T>
std::shared_ptr<T> Directory<Key, T>::lookup(const Key& keyId)
{
    QMutexLocker locker(&mutex);
    return callbackMap.value(keyId);
}

template <typename Key, typename T>
bool Directory<Key, T>::contains(const Key& keyId)
{
    QMutexLocker locker(&mutex);
    return callbackMap.contains(keyId);
}

template <typename Key, typename T>
// ownership passed off to the directory, which passes off to SharedPointer
void Directory<Key, T>::add(const Key& keyId, T* value)
{
    std::shared_ptr<T> valuePtr = std::shared_ptr<T>(value);
    add(keyId, valuePtr);
}

template <typename Key, typename T>
void Directory<Key, T>::add(const Key& keyId, std::shared_ptr<T> value)
{
    QMutexLocker locker(&mutex);
    callbackMap.insert(keyId, value);
}

// ownership passed off to the directory, which passes off to SharedPointer
template <typename Key, typename T>
void Directory<Key, T>::add(const Key& keyId, T* value, qint64 ttl_ms)
{
    std::shared_ptr<T> valuePtr = std::shared_ptr<T>(value);
    add(keyId, valuePtr, ttl_ms);
}

template <typename Key, typename T>
void Directory<Key, T>::add(const Key& keyId, std::shared_ptr<T> value, qint64 ttl_ms)
{
    // Insert the value
    {
        QMutexLocker locker(&mutex);
        callbackMap.insert(keyId, value);
    }

    // make a removerRunnable and shedule it to remove the entry after ttl!
    RemoverRunnable<Key, T>* removerRunnable = new RemoverRunnable<Key, T>(keyId, this);
    callBackRemoverScheduler.schedule(removerRunnable, ttl_ms);
}

template <typename Key, typename T>
void Directory<Key, T>::remove(const Key& keyId)
{
    QMutexLocker locker(&mutex);
    callbackMap.remove(keyId);
}

template <typename Key, typename T>
RemoverRunnable<Key, T>::RemoverRunnable(const Key& keyId, Directory<Key, T>* directory)
        : keyId(keyId), directory(directory)
{
}

template <typename Key, typename T>
void RemoverRunnable<Key, T>::run()
{
    //    LOG_TRACE(logger, "Calling Directory<Key,T>" );

    std::shared_ptr<T> val = directory->lookup(keyId);
    directory->remove(keyId);
}

/*
 * some objects that are put into the Directory need to be notified when the object is deleted
 * (e.g. the IReplyCaller will wait for a timeout from TTL). The RemoverRunnable therefore as a
 * partial specialisation for IReplyCallers and will call timeOut in this case.
 * This is also the reason why the IReplyCaller has to be in Common, instead of libjoynr/Proxy
 *
 */

template <typename Key>
class RemoverRunnable<Key, IReplyCaller> : public QRunnable
{
public:
    RemoverRunnable(const Key& keyId, Directory<Key, IReplyCaller>* directory);
    void run();

private:
    DISALLOW_COPY_AND_ASSIGN(RemoverRunnable);
    std::string keyId;
    Directory<Key, IReplyCaller>* directory;
    static joynr_logging::Logger* logger;
};

template <typename Key>
void RemoverRunnable<Key, IReplyCaller>::run()
{
    std::shared_ptr<IReplyCaller> value = directory->lookup(keyId);
    if (value) {
        value->timeOut();
        directory->remove(keyId);
    }
}

template <typename Key>
RemoverRunnable<Key, IReplyCaller>::RemoverRunnable(const Key& keyId,
                                                    Directory<Key, IReplyCaller>* directory)
        : keyId(keyId), directory(directory)
{
}

template <typename Key>
joynr_logging::Logger* RemoverRunnable<Key, IReplyCaller>::logger =
        joynr_logging::Logging::getInstance()->getLogger("MSG", "Directory");

} // namespace joynr

#ifndef STRING_QHASH
#define STRING_QHASH
namespace std
{
// using std::strings as key in a Directory requires qHash to be implemented
inline uint qHash(const std::string& key)
{
    return std::hash<std::string>()(key);
}
}
#endif // STRING_QHASH
#endif // DIRECTORY_H
