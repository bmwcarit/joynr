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

#include <type_traits>

#include <gtest/gtest.h>
#include <muesli/detail/CartesianTypeProduct.h>

#include "libjoynrclustercontroller/access-control/LocalDomainAccessStore.h"

using namespace ::testing;
using namespace joynr;
using namespace joynr::infrastructure::DacTypes;

namespace
{
namespace tags
{
// Testing functions
struct Domain
{
};
struct Interface
{
};

// Matching Type
struct ExactMatch
{
};
struct WildcardMatch
{
};
struct OnlyWildcardMatch
{
};
}

template <typename T>
struct IsACE : std::false_type
{
};
template <>
struct IsACE<MasterAccessControlEntry> : std::true_type
{
};
template <>
struct IsACE<OwnerAccessControlEntry> : std::true_type
{
};

template <typename T>
struct IsRCE : std::false_type
{
};
template <>
struct IsRCE<MasterRegistrationControlEntry> : std::true_type
{
};
template <>
struct IsRCE<OwnerRegistrationControlEntry> : std::true_type
{
};

template <typename FunctionType, typename ControlEntry>
struct BaseTestRunner
{
    BaseTestRunner() : userId("userId"), operationName("operationName")
    {
    }

    virtual ~BaseTestRunner() = default;

    template <typename T = FunctionType, typename U = ControlEntry>
    std::enable_if_t<std::is_same<T, tags::Domain>::value, void>
    completeInitializationOfExpectedEntry(U& entry)
    {
        entry.setDomain(getExpectedDomain());
        entry.setInterfaceName("DEFAULT_INTERFACE_NAME");
    }

    template <typename T = FunctionType, typename U = ControlEntry>
    std::enable_if_t<std::is_same<T, tags::Interface>::value, void>
    completeInitializationOfExpectedEntry(U& entry)
    {
        entry.setDomain("DEFAULT_DOMAIN");
        entry.setInterfaceName(getExpectedInterface());
    }

    template <typename T = FunctionType, typename U = ControlEntry>
    std::enable_if_t<std::is_same<T, tags::Domain>::value, void> completeInitializationOfQueryEntry(
            U& entry)
    {
        entry.setDomain(getQueryDomain());
        entry.setInterfaceName("DEFAULT_INTERFACE_NAME");
    }

    template <typename T = FunctionType, typename U = ControlEntry>
    std::enable_if_t<std::is_same<T, tags::Interface>::value, void>
    completeInitializationOfQueryEntry(U& entry)
    {
        entry.setDomain("DEFAULT_DOMAIN");
        entry.setInterfaceName(getQueryInterface());
    }

    template <typename T = ControlEntry>
    std::enable_if_t<IsACE<T>::value, T> getExpectedValue()
    {
        T value;
        value.setUid(userId);
        value.setOperation(operationName);
        completeInitializationOfExpectedEntry(value);
        return value;
    }

    template <typename T = ControlEntry>
    std::enable_if_t<IsRCE<T>::value, T> getExpectedValue()
    {
        T value;
        value.setUid(userId);
        completeInitializationOfExpectedEntry(value);
        return value;
    }

    template <typename T = ControlEntry>
    std::enable_if_t<IsACE<T>::value, T> getQueryValue()
    {
        T value;
        value.setUid(userId);
        completeInitializationOfQueryEntry(value);
        value.setOperation(operationName);
        return value;
    }

    template <typename T = ControlEntry>
    std::enable_if_t<IsRCE<T>::value, T> getQueryValue()
    {
        T value;
        value.setUid(userId);
        completeInitializationOfQueryEntry(value);
        return value;
    }

    template <typename T = ControlEntry>
    std::enable_if_t<std::is_same<T, MasterAccessControlEntry>::value> run()
    {
        const T expectedValue = getExpectedValue();
        const T queryValue = getQueryValue();

        this->localDomainAccessStore.updateMasterAccessControlEntry(expectedValue);
        boost::optional<MasterAccessControlEntry> queryResult =
                this->localDomainAccessStore.getMasterAccessControlEntry(
                        queryValue.getUid(),
                        queryValue.getDomain(),
                        queryValue.getInterfaceName(),
                        queryValue.getOperation());
        EXPECT_EQ(expectedValue, queryResult.get());

        EXPECT_TRUE(this->localDomainAccessStore.removeMasterAccessControlEntry(
                expectedValue.getUid(),
                expectedValue.getDomain(),
                expectedValue.getInterfaceName(),
                expectedValue.getOperation()));
    }

    template <typename T = ControlEntry>
    std::enable_if_t<std::is_same<T, OwnerAccessControlEntry>::value> run()
    {
        const T expectedValue = getExpectedValue();
        const T queryValue = getQueryValue();

        this->localDomainAccessStore.updateOwnerAccessControlEntry(expectedValue);
        boost::optional<OwnerAccessControlEntry> queryResult =
                this->localDomainAccessStore.getOwnerAccessControlEntry(
                        queryValue.getUid(),
                        queryValue.getDomain(),
                        queryValue.getInterfaceName(),
                        queryValue.getOperation());
        ASSERT_TRUE(queryResult);
        EXPECT_EQ(expectedValue, queryResult.get());

        EXPECT_TRUE(this->localDomainAccessStore.removeOwnerAccessControlEntry(
                expectedValue.getUid(),
                expectedValue.getDomain(),
                expectedValue.getInterfaceName(),
                expectedValue.getOperation()));
    }

    template <typename T = ControlEntry>
    std::enable_if_t<std::is_same<T, MasterRegistrationControlEntry>::value> run()
    {
        const T expectedValue = getExpectedValue();
        const T queryValue = getQueryValue();

        this->localDomainAccessStore.updateMasterRegistrationControlEntry(expectedValue);
        boost::optional<MasterRegistrationControlEntry> queryResult =
                this->localDomainAccessStore.getMasterRegistrationControlEntry(
                        queryValue.getUid(), queryValue.getDomain(), queryValue.getInterfaceName());
        ASSERT_TRUE(queryResult);
        EXPECT_EQ(expectedValue, queryResult.get());

        EXPECT_TRUE(this->localDomainAccessStore.removeMasterRegistrationControlEntry(
                expectedValue.getUid(),
                expectedValue.getDomain(),
                expectedValue.getInterfaceName()));
    }

    template <typename T = ControlEntry>
    std::enable_if_t<std::is_same<T, OwnerRegistrationControlEntry>::value> run()
    {
        const T expectedValue = getExpectedValue();
        const T queryValue = getQueryValue();

        this->localDomainAccessStore.updateOwnerRegistrationControlEntry(expectedValue);
        boost::optional<OwnerRegistrationControlEntry> queryResult =
                this->localDomainAccessStore.getOwnerRegistrationControlEntry(
                        queryValue.getUid(), queryValue.getDomain(), queryValue.getInterfaceName());
        ASSERT_TRUE(queryResult);
        EXPECT_EQ(expectedValue, queryResult.get());

        EXPECT_TRUE(this->localDomainAccessStore.removeOwnerRegistrationControlEntry(
                expectedValue.getUid(),
                expectedValue.getDomain(),
                expectedValue.getInterfaceName()));
    }

    virtual std::string getExpectedDomain()
    {
        return "domain/*";
    }

    virtual std::string getExpectedInterface()
    {
        return "interface/*";
    }

    virtual std::string getQueryInterface()
    {
        return "interface/123";
    }

    virtual std::string getQueryDomain()
    {
        return "domain/123";
    }

    LocalDomainAccessStore localDomainAccessStore;

    const std::string userId;
    const std::string operationName;
};

template <typename Matchingtype, typename FunctionType, typename ControlEntry>
struct TestRunner;

template <typename FunctionType, typename ControlEntry>
struct TestRunner<tags::WildcardMatch, FunctionType, ControlEntry>
        : public BaseTestRunner<FunctionType, ControlEntry>
{
};

template <typename FunctionType, typename ControlEntry>
struct TestRunner<tags::ExactMatch, FunctionType, ControlEntry>
        : public BaseTestRunner<FunctionType, ControlEntry>
{
    std::string getQueryInterface() override
    {
        return BaseTestRunner<FunctionType, ControlEntry>::getExpectedInterface();
    }

    std::string getQueryDomain() override
    {
        return BaseTestRunner<FunctionType, ControlEntry>::getExpectedDomain();
    }
};

template <typename FunctionType, typename ControlEntry>
struct TestRunner<tags::OnlyWildcardMatch, FunctionType, ControlEntry>
        : public BaseTestRunner<FunctionType, ControlEntry>
{
    std::string getExpectedDomain() override
    {
        return "*";
    }

    std::string getExpectedInterface() override
    {
        return "*";
    }
};

struct MakePair
{
    template <typename x, typename y>
    struct apply
    {
        using type = std::pair<x, y>;
    };
};
}

// Permutation space
using setFunctions = boost::mpl::vector<tags::Domain, tags::Interface>;
using matchingTypes =
        boost::mpl::vector<tags::ExactMatch, tags::WildcardMatch, tags::OnlyWildcardMatch>;
using controlEntries = boost::mpl::vector<MasterAccessControlEntry,
                                          OwnerAccessControlEntry,
                                          MasterRegistrationControlEntry,
                                          OwnerRegistrationControlEntry>;

// Generate all permutations
using TagTypeMplVector =
        muesli::detail::FlatCartesianTypeProduct<matchingTypes, setFunctions, MakePair>;
using AllCombinations =
        muesli::detail::FlatCartesianTypeProduct<TagTypeMplVector, controlEntries, MakePair>;

// Convert mpl vectors in a type list
using TagTypeList = muesli::detail::MplSequenceToTypeList<AllCombinations>::type;

template <typename TypeList>
struct TypeListToTestingTypes;

template <typename... Ts>
struct TypeListToTestingTypes<muesli::detail::TypeList<Ts...>>
{
    using type = ::testing::Types<Ts...>;
};

// List of types to pass to the test
using TagTypes = typename TypeListToTestingTypes<TagTypeList>::type;

template <typename Tag>
struct LocalDomainAccessStoreGenericTypedTest : public ::testing::Test
{
};

TYPED_TEST_SUITE(LocalDomainAccessStoreGenericTypedTest, TagTypes,);
TYPED_TEST(LocalDomainAccessStoreGenericTypedTest, updateAndRemoveDomainOrInterfaceWithWildCard)
{
    // ===============================================================
    // The type of the test (TypeParam) is:
    // pair<
    //     pair<
    //          functiontype,
    //          matchingtype
    //     >,
    //     controlEntry
    //    >
    // Out of TypeParam generate a TestRunnerType and call run on it.
    // ===============================================================

    using functionTypeAndMatchingType = typename TypeParam::first_type;
    using controlEntry = typename TypeParam::second_type;
    using functionType = typename functionTypeAndMatchingType::first_type;
    using matchingtype = typename functionTypeAndMatchingType::second_type;

    using TestRunnerType = TestRunner<functionType, matchingtype, controlEntry>;

    TestRunnerType testRunner;
    testRunner.run();
}
