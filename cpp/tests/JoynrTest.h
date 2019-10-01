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
#ifndef JOYNRTEST_H_
#define JOYNRTEST_H_

#include <exception>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "joynr/ImmutableMessage.h"
#include "joynr/MutableMessage.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/exceptions/MethodInvocationException.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvariadic-macros"
#define JOYNR_TEST_NO_THROW(statement, fail) \
    try { \
        statement; \
    } catch (joynr::exceptions::JoynrException& e) { \
        fail() << "Expected: " #statement " doesn't throw an exception.\n" \
                  "  Actual: it throws an " << e.getTypeName() << " with description \"" << e.getMessage() << "\""; \
    } catch (std::exception& e) { \
        fail() << "Expected: " #statement " doesn't throw an exception.\n" \
                  "  Actual: it throws an exception with description \"" << e.what() << "\""; \
    } catch (...) { \
        fail() << "Expected: " #statement " doesn't throw an exception.\n" \
                  "  Actual: it throws."; \
    }
#pragma GCC diagnostic pop

#define JOYNR_EXPECT_NO_THROW(statement) \
    JOYNR_TEST_NO_THROW(statement, ADD_FAILURE)

#define JOYNR_ASSERT_NO_THROW(statement) \
    JOYNR_TEST_NO_THROW(statement, FAIL)

ACTION_P(AcquireSemaphore, semaphore)
{
    semaphore->wait();
}

ACTION_P(ReleaseSemaphore, semaphore)
{
    semaphore->notify();
}

namespace joynr {
namespace test {
namespace util {

    /**
     * @brief Removes from the current directory all generated files.
     * It removes all *.settings, *.persist and *.entries files.
     */
    void removeAllCreatedSettingsAndPersistencyFiles();

    /**
     * @brief Remove specified file from current working directory.
     * @param filePattern: regex pattern to describe the set of files to remove.
     */
    void removeFileInCurrentDirectory(const std::string& filePattern);

    /**
     * @brief Copy the specified resource file from test-resources/ to current working directory.
     * @param resourceFileName: resource file to copy
     * @param newName: optional new name for the resource file (resourceFileName used otherwise)
     */
    void copyTestResourceToCurrentDirectory(const std::string& resourceFileName,
                                            const std::string& newName = std::string());

} // namespace util
} // namespace test
} // namespace joynr


inline void compareMutableImmutableMessage(const joynr::MutableMessage& mutableMessage, const joynr::ImmutableMessage& immutableMessage)
{
    // serialize MutableMessage and compare result with ImmutableMessage
    std::unique_ptr<joynr::ImmutableMessage> mutableMessageSerialized = mutableMessage.getImmutableMessage();
    EXPECT_EQ(mutableMessageSerialized->getSerializedMessage(), immutableMessage.getSerializedMessage());
}

inline void compareMutableImmutableMessage(const joynr::MutableMessage& mutableMessage, std::shared_ptr<joynr::ImmutableMessage> immutableMessage)
{
    compareMutableImmutableMessage(mutableMessage, *immutableMessage);
}

MATCHER_P(ImmutableMessageHasPayload, payload, "") {
    // we only support non-encrypted messages for now
    assert(!arg->isEncrypted());
    smrf::ByteArrayView bodyView = arg->getUnencryptedBody();
    const std::string immutablePayload(bodyView.data(), bodyView.data() + bodyView.size());

    return immutablePayload == payload;
}

MATCHER_P(ImmutableMessageHasPrefixedCustomHeaders, prefixedCustomHeaders, "") {
    auto msgCustomHeaders = arg->getCustomHeaders();
    for(const auto& customHeader : prefixedCustomHeaders) {
        auto keyIt = msgCustomHeaders.find(customHeader.first);

        if(keyIt == msgCustomHeaders.cend()) {
            return false;
        }

        if(keyIt->second != customHeader.second) {
            return false;
        }
    }

    return true;
}

// works for both Mutable and ImmutableMessages
MATCHER_P(MessageHasType, type, "") {
    return arg->getType() == type;
}

// works for both Mutable and ImmutableMessages
MATCHER_P(MessageHasSender, sender, "") {
    return arg->getSender() == sender;
}

// works for both Mutable and ImmutableMessages
MATCHER_P(MessageHasRecipient, recipient, "") {
    return arg->getRecipient() == recipient;
}

// works for both Mutable and ImmutableMessages
MATCHER_P(MessageHasExpiryDate, expiryDate, "") {
    return arg->getExpiryDate() == expiryDate;
}

MATCHER_P2(joynrException, typeName, msg,
           "JoynrException with typeName=" + ::testing::PrintToString(typeName)
           + " and message=" + ::testing::PrintToString(msg)) {
    return arg.getTypeName() == typeName && arg.getMessage() == msg;
}

#endif // JOYNRTEST_H_
