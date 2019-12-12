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

#include "tests/systemintegration-tests/TestConfiguration.h"

using namespace joynr::tests;

Configuration& Configuration::getInstance()
{
    static Configuration* instance = new Configuration();
    return *instance;
}

Configuration::Configuration() : defaultSystemSettingsFile(), defaultWebsocketSettingsFile()
{
}

Configuration::~Configuration()
{
}

void Configuration::setDefaultSystemSettingsFile(const std::string& filename)
{
    defaultSystemSettingsFile = filename;
}

void Configuration::setDefaultWebsocketSettingsFile(const std::string& filename)
{
    defaultWebsocketSettingsFile = filename;
}

const std::string& Configuration::getDefaultSystemSettingsFile() const
{
    return defaultSystemSettingsFile;
}

const std::string& Configuration::getDefaultWebsocketSettingsFile() const
{
    return defaultWebsocketSettingsFile;
}
