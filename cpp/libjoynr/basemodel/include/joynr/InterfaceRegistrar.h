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
#ifndef INTERFACEREGISTRAR_H
#define INTERFACEREGISTRAR_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "joynr/JoynrExport.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

class IRequestInterpreter;

/**
 * Registers RequestInterpreters for generated interfaces.
 *
 * An application or library can register its interfaces with this registrar and then
 * libjoynr will use the given instances of the RequestInterpreters
 * to call the RequestCallers that implement the interfaces.
 */
class JOYNR_EXPORT InterfaceRegistrar
{
public:
    /**
     * This class is currently implemented as a singleton
     */
    static InterfaceRegistrar& instance();

    /**
     * Register a request interpreter of type T for the interface with the given name
     */
    template <class T>
    void registerRequestInterpreter(const std::string& interfaceName);

    /**
     * Unregister a request interpreter for the given interface
     */
    void unregisterRequestInterpreter(const std::string& interfaceName);

    /**
     * Get a request interpreter for the given interface
     */
    std::shared_ptr<IRequestInterpreter> getRequestInterpreter(const std::string& interfaceName);

    /**
     * Reset the InterfaceRegistrar - for use in tests
     */
    void reset();

private:
    InterfaceRegistrar();
    DISALLOW_COPY_AND_ASSIGN(InterfaceRegistrar);

    // Thread safe hash table of request interpreters
    std::unordered_map<std::string, std::shared_ptr<IRequestInterpreter>> requestInterpreters;
    std::mutex requestInterpretersMutex;

    // A count of how many registrations are done for each request interpreter
    // Also protected by requestInterpretersMutex
    std::unordered_map<std::string, int> requestInterpreterCounts;
};

template <class T>
void InterfaceRegistrar::registerRequestInterpreter(const std::string& interfaceName)
{
    std::lock_guard<std::mutex> lock(requestInterpretersMutex);
    if (requestInterpreters.find(interfaceName) == requestInterpreters.end()) {
        requestInterpreters.insert({interfaceName, std::make_shared<T>()});
        requestInterpreterCounts.insert({interfaceName, 1});
    } else {
        ++requestInterpreterCounts[interfaceName];
    }
}

} // namespace joynr
#endif // INTERFACEREGISTRAR_H
