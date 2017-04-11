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

#include <joynr/exceptions/JoynrException.h>
#include <joynr/exceptions/MethodInvocationException.h>

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

#endif // JOYNRTEST_H_
