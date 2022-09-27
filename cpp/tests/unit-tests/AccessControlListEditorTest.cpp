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

#include <chrono>
#include <memory>

#include "tests/utils/Gmock.h"
#include "tests/utils/Gtest.h"

#include "libjoynrclustercontroller/access-control/AccessControlListEditor.h"

#include "joynr/CallContext.h"
#include "joynr/CallContextStorage.h"
#include "joynr/Semaphore.h"
#include "joynr/exceptions/JoynrException.h"
#include "joynr/infrastructure/DacTypes/DomainRoleEntry.h"
#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

#include "tests/mock/MockLocalDomainAccessController.h"

using namespace joynr;
using ::testing::Return;

class MockLocalDomainAccessStore : public LocalDomainAccessStore
{
public:
    MOCK_METHOD1(updateMasterAccessControlEntry,
                 bool(const infrastructure::DacTypes::MasterAccessControlEntry& updatedMasterAce));
    MOCK_METHOD4(removeMasterAccessControlEntry,
                 bool(const std::string& userId,
                      const std::string& domain,
                      const std::string& interfaceName,
                      const std::string& operation));

    MOCK_METHOD1(
            updateMediatorAccessControlEntry,
            bool(const infrastructure::DacTypes::MasterAccessControlEntry& updatedMediatorAce));
    MOCK_METHOD4(removeMediatorAccessControlEntry,
                 bool(const std::string& userId,
                      const std::string& domain,
                      const std::string& interfaceName,
                      const std::string& operation));

    MOCK_METHOD1(updateOwnerAccessControlEntry,
                 bool(const infrastructure::DacTypes::OwnerAccessControlEntry& updatedOwnerAce));
    MOCK_METHOD4(removeOwnerAccessControlEntry,
                 bool(const std::string& userId,
                      const std::string& domain,
                      const std::string& interfaceName,
                      const std::string& operation));

    MOCK_METHOD1(
            updateMasterRegistrationControlEntry,
            bool(const infrastructure::DacTypes::MasterRegistrationControlEntry& updatedMasterRce));
    MOCK_METHOD3(removeMasterRegistrationControlEntry,
                 bool(const std::string& uid,
                      const std::string& domain,
                      const std::string& interfaceName));

    MOCK_METHOD1(updateMediatorRegistrationControlEntry,
                 bool(const infrastructure::DacTypes::MasterRegistrationControlEntry&
                              updatedMediatorRce));
    MOCK_METHOD3(removeMediatorRegistrationControlEntry,
                 bool(const std::string& uid,
                      const std::string& domain,
                      const std::string& interfaceName));

    MOCK_METHOD1(
            updateOwnerRegistrationControlEntry,
            bool(const infrastructure::DacTypes::OwnerRegistrationControlEntry& updatedOwnerRce));
    MOCK_METHOD3(removeOwnerRegistrationControlEntry,
                 bool(const std::string& uid,
                      const std::string& domain,
                      const std::string& interfaceName));
};

class AccessControlListEditorTest : public testing::Test
{
public:
    AccessControlListEditorTest()
            : mockLocalDomainAccessStore(std::make_shared<MockLocalDomainAccessStore>()),
              mockLocalDomainAccessController(std::make_shared<MockLocalDomainAccessController>(
                      mockLocalDomainAccessStore)),
              aclEditor(mockLocalDomainAccessStore, mockLocalDomainAccessController, false),
              semaphore(0)
    {
        onSuccessExpectTrue = [this](const bool& success) {
            EXPECT_TRUE(success);
            semaphore.notify();
        };

        onSuccessExpectFalse = [this](const bool& success) {
            EXPECT_FALSE(success);
            semaphore.notify();
        };

        onErrorFail = [](const exceptions::ProviderRuntimeException& error) {
            FAIL() << "onFailure called with error " << error.getMessage();
        };
    }

protected:
    void setExpectationForCallToHasRole(const infrastructure::DacTypes::Role::Enum& role,
                                        const bool returnValue)
    {
        CallContext callContext;
        callContext.setPrincipal(testUidCallContext);
        CallContextStorage::set(std::move(callContext));
        EXPECT_CALL(*mockLocalDomainAccessController, hasRole(testUidCallContext, testDomain, role))
                .Times(1)
                .WillOnce(Return(returnValue));
    }

    const joynr::infrastructure::DacTypes::MasterAccessControlEntry createMasterAce()
    {
        const std::vector<infrastructure::DacTypes::Permission::Enum> permissions;
        const std::vector<infrastructure::DacTypes::TrustLevel::Enum> trustLevels;
        const joynr::infrastructure::DacTypes::MasterAccessControlEntry masterAce(
                testUid,
                testDomain,
                testInterfaceName,
                infrastructure::DacTypes::TrustLevel::LOW,
                trustLevels,
                infrastructure::DacTypes::TrustLevel::LOW,
                trustLevels,
                testOperation,
                infrastructure::DacTypes::Permission::NO,
                permissions);
        return masterAce;
    }

    const joynr::infrastructure::DacTypes::OwnerAccessControlEntry createOwnerAce()
    {
        const joynr::infrastructure::DacTypes::OwnerAccessControlEntry ownerAce(
                testUid,
                testDomain,
                testInterfaceName,
                infrastructure::DacTypes::TrustLevel::LOW,
                infrastructure::DacTypes::TrustLevel::LOW,
                testOperation,
                infrastructure::DacTypes::Permission::NO);
        return ownerAce;
    }

    const joynr::infrastructure::DacTypes::MasterRegistrationControlEntry createMasterRce()
    {
        const std::vector<infrastructure::DacTypes::Permission::Enum> permissions;
        const std::vector<infrastructure::DacTypes::TrustLevel::Enum> trustLevels;
        const joynr::infrastructure::DacTypes::MasterRegistrationControlEntry masterRce(
                testUid,
                testDomain,
                testInterfaceName,
                infrastructure::DacTypes::TrustLevel::LOW,
                trustLevels,
                infrastructure::DacTypes::TrustLevel::LOW,
                trustLevels,
                infrastructure::DacTypes::Permission::NO,
                permissions);
        return masterRce;
    }

    const joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry createOwnerRce()
    {
        const joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry ownerRce(
                testUid,
                testDomain,
                testInterfaceName,
                infrastructure::DacTypes::TrustLevel::LOW,
                infrastructure::DacTypes::TrustLevel::LOW,
                infrastructure::DacTypes::Permission::NO);
        return ownerRce;
    }

    std::shared_ptr<MockLocalDomainAccessStore> mockLocalDomainAccessStore;
    std::shared_ptr<MockLocalDomainAccessController> mockLocalDomainAccessController;
    AccessControlListEditor aclEditor;

    Semaphore semaphore;
    std::function<void(const bool&)> onSuccessExpectTrue;
    std::function<void(const bool&)> onSuccessExpectFalse;
    std::function<void(const exceptions::ProviderRuntimeException&)> onErrorFail;

    const static std::string testUid;
    const static std::string testUidCallContext;
    const static std::string testDomain;
    const static std::string testInterfaceName;
    const static std::string testOperation;
};

const std::string AccessControlListEditorTest::testUid = "testUid";
const std::string AccessControlListEditorTest::testUidCallContext = "testUidCallContext";
const std::string AccessControlListEditorTest::testDomain = "testDomain";
const std::string AccessControlListEditorTest::testInterfaceName = "testInterface";
const std::string AccessControlListEditorTest::testOperation = "testOperation";

TEST_F(AccessControlListEditorTest, updateMasterAccessControlEntry)
{
    const joynr::infrastructure::DacTypes::MasterAccessControlEntry expectedMasterAce =
            createMasterAce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, true);

    EXPECT_CALL(*mockLocalDomainAccessStore, updateMasterAccessControlEntry(expectedMasterAce))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.updateMasterAccessControlEntry(expectedMasterAce, onSuccessExpectTrue, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, updateMasterAccessControlEntryWithoutPermission)
{
    const joynr::infrastructure::DacTypes::MasterAccessControlEntry expectedMasterAce =
            createMasterAce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, false);

    EXPECT_CALL(*mockLocalDomainAccessStore, updateMasterAccessControlEntry(expectedMasterAce))
            .Times(0);

    aclEditor.updateMasterAccessControlEntry(expectedMasterAce, onSuccessExpectFalse, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeMasterAccessControlEntry)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, true);

    EXPECT_CALL(
            *mockLocalDomainAccessStore,
            removeMasterAccessControlEntry(testUid, testDomain, testInterfaceName, testOperation))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.removeMasterAccessControlEntry(testUid,
                                             testDomain,
                                             testInterfaceName,
                                             testOperation,
                                             onSuccessExpectTrue,
                                             onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeMasterAccessControlEntryWithoutPermission)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, false);

    EXPECT_CALL(
            *mockLocalDomainAccessStore,
            removeMasterAccessControlEntry(testUid, testDomain, testInterfaceName, testOperation))
            .Times(0);

    aclEditor.removeMasterAccessControlEntry(testUid,
                                             testDomain,
                                             testInterfaceName,
                                             testOperation,
                                             onSuccessExpectFalse,
                                             onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, updateMediatorAccessControlEntry)
{
    const joynr::infrastructure::DacTypes::MasterAccessControlEntry expectedMediatorAce =
            createMasterAce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, true);

    EXPECT_CALL(*mockLocalDomainAccessStore, updateMediatorAccessControlEntry(expectedMediatorAce))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.updateMediatorAccessControlEntry(
            expectedMediatorAce, onSuccessExpectTrue, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, updateMediatorAccessControlEntryWithoutPermission)
{
    const joynr::infrastructure::DacTypes::MasterAccessControlEntry expectedMediatorAce =
            createMasterAce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, false);

    EXPECT_CALL(*mockLocalDomainAccessStore, updateMediatorAccessControlEntry(expectedMediatorAce))
            .Times(0);

    aclEditor.updateMediatorAccessControlEntry(
            expectedMediatorAce, onSuccessExpectFalse, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeMediatorAccessControlEntry)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, true);

    EXPECT_CALL(
            *mockLocalDomainAccessStore,
            removeMediatorAccessControlEntry(testUid, testDomain, testInterfaceName, testOperation))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.removeMediatorAccessControlEntry(testUid,
                                               testDomain,
                                               testInterfaceName,
                                               testOperation,
                                               onSuccessExpectTrue,
                                               onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeMediatorAccessControlEntryWithoutPermission)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, false);

    EXPECT_CALL(
            *mockLocalDomainAccessStore,
            removeMediatorAccessControlEntry(testUid, testDomain, testInterfaceName, testOperation))
            .Times(0);

    aclEditor.removeMediatorAccessControlEntry(testUid,
                                               testDomain,
                                               testInterfaceName,
                                               testOperation,
                                               onSuccessExpectFalse,
                                               onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, updateOwnerAccessControlEntry)
{
    const joynr::infrastructure::DacTypes::OwnerAccessControlEntry expectedOwnerAce =
            createOwnerAce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::OWNER, true);

    EXPECT_CALL(*mockLocalDomainAccessStore, updateOwnerAccessControlEntry(expectedOwnerAce))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.updateOwnerAccessControlEntry(expectedOwnerAce, onSuccessExpectTrue, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, updateOwnerAccessControlEntryWithoutPermission)
{
    const joynr::infrastructure::DacTypes::OwnerAccessControlEntry expectedOwnerAce =
            createOwnerAce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::OWNER, false);

    EXPECT_CALL(*mockLocalDomainAccessStore, updateOwnerAccessControlEntry(expectedOwnerAce))
            .Times(0);

    aclEditor.updateOwnerAccessControlEntry(expectedOwnerAce, onSuccessExpectFalse, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeOwnerAccessControlEntry)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::OWNER, true);

    EXPECT_CALL(
            *mockLocalDomainAccessStore,
            removeOwnerAccessControlEntry(testUid, testDomain, testInterfaceName, testOperation))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.removeOwnerAccessControlEntry(testUid,
                                            testDomain,
                                            testInterfaceName,
                                            testOperation,
                                            onSuccessExpectTrue,
                                            onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeOwnerAccessControlEntryWithoutPermission)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::OWNER, false);

    EXPECT_CALL(
            *mockLocalDomainAccessStore,
            removeOwnerAccessControlEntry(testUid, testDomain, testInterfaceName, testOperation))
            .Times(0);

    aclEditor.removeOwnerAccessControlEntry(testUid,
                                            testDomain,
                                            testInterfaceName,
                                            testOperation,
                                            onSuccessExpectFalse,
                                            onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, updateMasterRegistrationControlEntry)
{
    const joynr::infrastructure::DacTypes::MasterRegistrationControlEntry expectedMasterRce =
            createMasterRce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, true);

    EXPECT_CALL(
            *mockLocalDomainAccessStore, updateMasterRegistrationControlEntry(expectedMasterRce))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.updateMasterRegistrationControlEntry(
            expectedMasterRce, onSuccessExpectTrue, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, updateMasterRegistrationControlEntryWithoutPermission)
{
    const joynr::infrastructure::DacTypes::MasterRegistrationControlEntry expectedMasterRce =
            createMasterRce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, false);

    EXPECT_CALL(
            *mockLocalDomainAccessStore, updateMasterRegistrationControlEntry(expectedMasterRce))
            .Times(0);

    aclEditor.updateMasterRegistrationControlEntry(
            expectedMasterRce, onSuccessExpectFalse, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeMasterRegistrationControlEntry)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, true);

    EXPECT_CALL(*mockLocalDomainAccessStore,
                removeMasterRegistrationControlEntry(testUid, testDomain, testInterfaceName))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.removeMasterRegistrationControlEntry(
            testUid, testDomain, testInterfaceName, onSuccessExpectTrue, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeMasterRegistrationControlEntryWithoutPermission)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, false);

    EXPECT_CALL(*mockLocalDomainAccessStore,
                removeMasterRegistrationControlEntry(testUid, testDomain, testInterfaceName))
            .Times(0);

    aclEditor.removeMasterRegistrationControlEntry(
            testUid, testDomain, testInterfaceName, onSuccessExpectFalse, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, updateMediatorRegistrationControlEntry)
{
    const joynr::infrastructure::DacTypes::MasterRegistrationControlEntry expectedMediatorRce =
            createMasterRce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, true);

    EXPECT_CALL(*mockLocalDomainAccessStore,
                updateMediatorRegistrationControlEntry(expectedMediatorRce))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.updateMediatorRegistrationControlEntry(
            expectedMediatorRce, onSuccessExpectTrue, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, updateMediatorRegistrationControlEntryWithoutPermission)
{
    const joynr::infrastructure::DacTypes::MasterRegistrationControlEntry expectedMediatorRce =
            createMasterRce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, false);

    EXPECT_CALL(*mockLocalDomainAccessStore,
                updateMediatorRegistrationControlEntry(expectedMediatorRce))
            .Times(0);

    aclEditor.updateMediatorRegistrationControlEntry(
            expectedMediatorRce, onSuccessExpectFalse, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeMediatorRegistrationControlEntry)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, true);

    EXPECT_CALL(*mockLocalDomainAccessStore,
                removeMediatorRegistrationControlEntry(testUid, testDomain, testInterfaceName))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.removeMediatorRegistrationControlEntry(
            testUid, testDomain, testInterfaceName, onSuccessExpectTrue, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeMediatorRegistrationControlEntryWithoutPermission)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::MASTER, false);

    EXPECT_CALL(*mockLocalDomainAccessStore,
                removeMediatorRegistrationControlEntry(testUid, testDomain, testInterfaceName))
            .Times(0);

    aclEditor.removeMediatorRegistrationControlEntry(
            testUid, testDomain, testInterfaceName, onSuccessExpectFalse, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, updateOwnerRegistrationControlEntry)
{
    const joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry expectedOwnerRce =
            createOwnerRce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::OWNER, true);

    EXPECT_CALL(*mockLocalDomainAccessStore, updateOwnerRegistrationControlEntry(expectedOwnerRce))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.updateOwnerRegistrationControlEntry(
            expectedOwnerRce, onSuccessExpectTrue, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, updateOwnerRegistrationControlEntryWithoutPermission)
{
    const joynr::infrastructure::DacTypes::OwnerRegistrationControlEntry expectedOwnerRce =
            createOwnerRce();

    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::OWNER, false);

    EXPECT_CALL(*mockLocalDomainAccessStore, updateOwnerRegistrationControlEntry(expectedOwnerRce))
            .Times(0);

    aclEditor.updateOwnerRegistrationControlEntry(
            expectedOwnerRce, onSuccessExpectFalse, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeOwnerRegistrationControlEntry)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::OWNER, true);

    EXPECT_CALL(*mockLocalDomainAccessStore,
                removeOwnerRegistrationControlEntry(testUid, testDomain, testInterfaceName))
            .Times(1)
            .WillOnce(Return(true));

    aclEditor.removeOwnerRegistrationControlEntry(
            testUid, testDomain, testInterfaceName, onSuccessExpectTrue, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}

TEST_F(AccessControlListEditorTest, removeOwnerRegistrationControlEntryWithoutPermission)
{
    setExpectationForCallToHasRole(infrastructure::DacTypes::Role::OWNER, false);

    EXPECT_CALL(*mockLocalDomainAccessStore,
                removeOwnerRegistrationControlEntry(testUid, testDomain, testInterfaceName))
            .Times(0);

    aclEditor.removeOwnerRegistrationControlEntry(
            testUid, testDomain, testInterfaceName, onSuccessExpectFalse, onErrorFail);

    EXPECT_TRUE(semaphore.waitFor(std::chrono::milliseconds(1000)));
}
