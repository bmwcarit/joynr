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

#ifndef SIT_UTIL_H
#define SIT_UTIL_H

#include <memory>

namespace joynr
{

class JoynrRuntime;

namespace sitUtil
{
std::shared_ptr<joynr::JoynrRuntime> createRuntime(const std::string& pathToSettings,
                                                   const std::string& sslCertFilename,
                                                   const std::string& sslPrivateKeyFilename,
                                                   const std::string& sslCaCertFilename);
}

} // namespace joynr

#endif // SIT_UTIL_H
