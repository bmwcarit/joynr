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

#include "joynr/Settings.h"

#include <boost/property_tree/ini_parser.hpp>
#include <utility>

#include "joynr/Util.h"

namespace ptree = boost::property_tree;

namespace joynr
{

Settings::Settings() : _filename(), _propertyTree(), _loaded(false)
{
}

Settings::Settings(const std::string& filename, bool throwIfError)
        : _filename(filename), _propertyTree(), _loaded(false)
{
    try {
        ptree::read_ini(filename, _propertyTree);
        _loaded = true;
        JOYNR_LOG_INFO(logger(), "Attempt reading {} succeeded.", filename);
    } catch (const ptree::ini_parser_error& e) {
        if (throwIfError) {
            throw;
        }
        JOYNR_LOG_INFO(logger(), "Could not read properties from {}.", filename);
    }
}

bool Settings::isLoaded() const
{
    return _loaded;
}

bool Settings::contains(const std::string& path) const
{
    // Create a '/' delimited path
    ptree::ptree::path_type treePath = createPath(path);

    // Get the child node with the path
    const auto& child = _propertyTree.get_child_optional(treePath);

    // Use boost::optional operator bool()
    return bool(child);
}

bool Settings::sync()
{
    try {
        if (contentChanged(_propertyTree, _filename)) {
            ptree::write_ini(_filename, _propertyTree);
            return true;
        } else {
            JOYNR_LOG_INFO(logger(),
                           "Settings file \"{}\" not updated because its content did not change",
                           _filename);
        }
    } catch (const ptree::ini_parser_error& e) {
        JOYNR_LOG_ERROR(logger(),
                        "settings file \"{}\" cannot be written due to the following error: {}",
                        _filename,
                        e.message());
    }
    return false;
}

void Settings::merge(const Settings& from, Settings& to, bool overwrite)
{
    const ptree::ptree& fromTree = from._propertyTree;
    ptree::ptree& toTree = to._propertyTree;

    merge(fromTree, toTree, overwrite);
    to._loaded = true;
}

bool Settings::contentChanged(const boost::property_tree::ptree& propertyTreeToCompare,
                              const std::string& iniFilename) const
{
    if (!util::fileExists(iniFilename)) {
        return true;
    }

    boost::property_tree::ptree otherPropertyTree;

    try {
        ptree::read_ini(iniFilename, otherPropertyTree);
    } catch (const ptree::ini_parser_error&) {
        return true;
    }

    return propertyTreeToCompare != otherPropertyTree;
}

void Settings::fillEmptySettingsWithDefaults(const std::string& defaultsFilename)
{
    const std::string cmakeSettingsPath = CMAKE_JOYNR_SETTINGS_INSTALL_DIR;
    const std::string absolutePath = cmakeSettingsPath + "/" + defaultsFilename;
    const std::string relativePath = "resources/" + defaultsFilename;
    Settings cmakeDefaultSettings;
    Settings relativeDefaultSettings;
    bool loadedAbsolute = false;
    try {
        Settings::merge(Settings(absolutePath, true), cmakeDefaultSettings, true);
        loadedAbsolute = true;
        JOYNR_LOG_INFO(logger(), "Attempt reading {} succeeded.", absolutePath);
    } catch (const ptree::ini_parser_error&) {
    }

    bool loadedRelative = false;
    try {
        Settings::merge(Settings(relativePath, true), relativeDefaultSettings, true);
        loadedRelative = true;
        JOYNR_LOG_INFO(logger(), "Attempt reading {} succeeded.", relativePath);
    } catch (const ptree::ini_parser_error&) {
    }

    if (loadedAbsolute || loadedRelative) {
        Settings::merge(relativeDefaultSettings, *this, false);
        Settings::merge(cmakeDefaultSettings, *this, false);
        _loaded = true;
    }

    if (!loadedAbsolute && !loadedRelative) {
        JOYNR_LOG_ERROR(logger(),
                        "Could not read properties from absolute path {} and relative path {}",
                        absolutePath,
                        relativePath);
    }
}

void Settings::merge(const boost::property_tree::ptree& from,
                     boost::property_tree::ptree& to,
                     bool overwrite)
{
    // Is this a single value or a subtree?
    if (!from.data().empty()) {
        // Single value
        if (overwrite || to.data().empty()) {
            to.put_value(from.data());
        }
        return;
    }

    // Subtree
    for (const auto& fromEntry : from) {
        // Does the key exist in the destination?
        auto toIt = to.find(fromEntry.first);
        if (toIt == to.not_found()) {
            ptree::ptree child;

            // Recurse into the new child
            merge(fromEntry.second, child, overwrite);

            // Create a path object because ptree uses '.' as a path delimiter
            // when strings are used
            ptree::ptree::path_type treePath = createPath(fromEntry.first);
            to.add_child(treePath, child);
        } else {
            // Recurse into the subtrees
            merge(fromEntry.second, toIt->second, overwrite);
        }
    }
}

boost::property_tree::path Settings::createPath(const std::string& path)
{
    return boost::property_tree::path(path, '/');
}

} // namespace joynr
