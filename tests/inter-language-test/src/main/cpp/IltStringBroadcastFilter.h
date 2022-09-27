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
#ifndef ILTSTRINGBROADCASTFILTER_H
#define ILTSTRINGBROADCASTFILTER_H

#include "joynr/Logger.h"
#include "joynr/interlanguagetest/TestInterfaceBroadcastWithFilteringBroadcastFilter.h"
#include "joynr/interlanguagetest/TestInterfaceBroadcastWithFilteringBroadcastFilterParameters.h"

using namespace joynr;

class IltStringBroadcastFilter
        : public interlanguagetest::TestInterfaceBroadcastWithFilteringBroadcastFilter
{
public:
    IltStringBroadcastFilter();

    virtual bool filter(
            const std::string& stringOut,
            const std::vector<std::string>& stringArrayOut,
            const joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedTypeCollectionEnumerationInTypeCollection::Enum& enumerationOut,
            const joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray&
                    structWithStringArrayOut,
            const std::vector<
                    joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                    structWithStringArrayArrayOut,
            const interlanguagetest::TestInterfaceBroadcastWithFilteringBroadcastFilterParameters&
                    filterParameters);

private:
    ADD_LOGGER(IltStringBroadcastFilter)
};

#endif // ILTSTRINGBROADCASTFILTER_H
