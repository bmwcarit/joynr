/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2016 BMW Car IT GmbH
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

#include <gtest/gtest.h>

#include "joynr/CapabilityEntry.h"
using namespace joynr;

TEST(CapabilityEntryTest, testEqualsOperator)
{
    CapabilityEntry entry1;
    CapabilityEntry entry2;
    EXPECT_EQ(entry1, entry2);
    entry1.setProviderVersion(joynr::types::Version(2,2));
    EXPECT_FALSE(entry1 == entry2);
}
