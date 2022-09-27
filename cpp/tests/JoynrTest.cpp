/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
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

#include <fstream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/regex.hpp>

namespace joynr
{
namespace test
{
namespace util
{

void removeFileInCurrentDirectory(const std::string& filePattern)
{
    boost::filesystem::path currentDir(".");
    boost::regex pattern(filePattern);
    boost::filesystem::directory_iterator dirBegin(currentDir), dirEnd;

    BOOST_FOREACH (const boost::filesystem::path& file, std::make_pair(dirBegin, dirEnd)) {
        if (boost::filesystem::is_regular_file(file)) {
            std::string fileName = file.filename().string();
            boost::smatch result;
            if (boost::regex_match(fileName, result, pattern)) {
                boost::filesystem::remove(file);
            }
        }
    }
}

void removeAllCreatedSettingsAndPersistencyFiles()
{
    removeFileInCurrentDirectory(".*\\.settings");
    removeFileInCurrentDirectory(".*\\.persist");
    removeFileInCurrentDirectory(".*\\.entries");
}

void copyTestResourceToCurrentDirectory(const std::string& resourceFileName,
                                        const std::string& newName)
{
    static const std::string TEST_RESOURCE_FOLDER = "test-resources/";
    const std::ifstream src(TEST_RESOURCE_FOLDER + resourceFileName);
    std::ofstream dst;

    if (newName.empty()) {
        dst.open(resourceFileName);
    } else {
        dst.open(newName);
    }

    assert(src.is_open());
    assert(dst.is_open());

    dst << src.rdbuf();
}

} // namespace util
} // namespace test
} // namespace joynr
