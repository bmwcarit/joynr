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
#ifndef JOYNR_SETTINGS_H
#define JOYNR_SETTINGS_H

#include <string>

#include <boost/optional.hpp>
#include <boost/property_tree/ptree.hpp>

#include "joynr/JoynrExport.h"
#include "joynr/Logger.h"
#include "joynr/PrivateCopyAssign.h"

namespace joynr
{

/**
 * @brief Class that encapsulates settings used by Joynr
 */
class JOYNR_EXPORT Settings
{
public:
    /**
     * @brief Create an empty settings object
     */
    Settings();

    /**
     * @brief Read settings from a file
     * @param filename The name of the file to read settings from
     */
    explicit Settings(const std::string& filename, bool throwIfError = false);

    /**
     * @brief Indicates if the settings file was parsed and loaded
     * @return  true if the file was parsed and loaded, false if not
     */
    bool isLoaded() const;

    /**
     * @brief Do the settings contain the given value at the given path?
     * @param path The path of the value
     * @return true if the settins contains the given path, false otherwise
     */
    bool contains(const std::string& path) const;

    /**
     * @brief Get the setting with the given path
     * @param path The '/' delimited path to get the setting for
     */
    template <class T>
    T get(const std::string& path) const;

    /**
     * @brief Get the setting with the given path
     * @param path The '/' delimited path to get the setting for
     */
    template <class T>
    boost::optional<T> getOptional(const std::string& path) const;

    /**
     * @brief Set the setting with the given path, overwriting any previous value.
     * @param path The '/' delimited path to the setting
     * @param value The value to set
     */
    template <class T>
    void set(const std::string& path, const T& value);

    /**
     * @brief Sync changes to disk
     * @return Returns true when new content was written to the disk. Returns false
     * if an error occured or if the content did not change (therefore, don't
     * use the return value as an error indication)
     */
    bool sync();

    /**
     * @brief Merge two settings objects.
     *
     * This function is the responsibility of the Settings class because
     * it requires knowledge of the settings implementation.
     *
     * @param from The object to merge
     * @param to The destination object
     * @param overwrite Should existing paths in 'to' be overwritten?
     */
    static void merge(const Settings& from, Settings& to, bool overwrite);

    /**
     * @brief Any empty setting (any settings which are not already set) will be first filled from
     * "resources/$defaultsFilename"
     *        and afterwards from the location given by
     * "CMAKE_JOYNR_SETTINGS_INSTALL_DIR/$defaultsFilename".
     * @param defaultsFilename The name of the file where the defaults are read from
     */
    void fillEmptySettingsWithDefaults(const std::string& defaultsFilename);

private:
    DISALLOW_COPY_AND_ASSIGN(Settings);

    std::string _filename;
    boost::property_tree::ptree _propertyTree;
    bool _loaded;

    static void merge(const boost::property_tree::ptree& from,
                      boost::property_tree::ptree& to,
                      bool overwrite);
    bool contentChanged(const boost::property_tree::ptree& propertyTreeToCompare,
                        const std::string& iniFilename) const;

    static boost::property_tree::path createPath(const std::string& path);
    ADD_LOGGER(Settings)
};

//---- Implementation ----------------------------------------------------------

template <class T>
T Settings::get(const std::string& path) const
{
    // Create a '/' delimited path
    boost::property_tree::path treePath = createPath(path);

    // Get the value with the path
    return _propertyTree.get<T>(treePath, T());
}

template <class T>
boost::optional<T> Settings::getOptional(const std::string& path) const
{
    // Create a '/' delimited path
    const boost::property_tree::path treePath = createPath(path);

    // Get the value with the path
    return _propertyTree.get_optional<T>(treePath);
}

template <class T>
void Settings::set(const std::string& path, const T& value)
{
    // Create a '/' delimited path
    boost::property_tree::path treePath = createPath(path);

    _propertyTree.put(treePath, value);
}

} // namespace joynr
#endif // JOYNR_SETTINGS_H
