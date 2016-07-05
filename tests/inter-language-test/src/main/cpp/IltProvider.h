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
#ifndef MY_PROVIDER_H
#define MY_PROVIDER_H

#include "joynr/interlanguagetest/DefaultTestInterfaceProvider.h"
#include "joynr/Logger.h"

/**
  * A TestInterface Provider with a circular list of radio stations
  */
class IltProvider : public joynr::interlanguagetest::DefaultTestInterfaceProvider
{
public:
    IltProvider();
    ~IltProvider();

    /*
     * Implementation of Getters for Franca attributes
     */

    /*
     * Implementation of Franca methods
     */

    virtual void methodWithoutParameters(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithoutInputParameter(
            std::function<void(const bool& booleanOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithoutOutputParameter(
            const bool& booleanArg,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithSinglePrimitiveParameters(
            const uint16_t& uInt16Arg,
            std::function<void(const std::string& stringOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithMultiplePrimitiveParameters(
            const int32_t& int32Arg,
            const float& floatArg,
            const bool& booleanArg,
            std::function<void(const double& doubleOut, const std::string& stringOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithSingleArrayParameters(
            const std::vector<double>& doubleArrayArg,
            std::function<void(const std::vector<std::string>& stringArrayOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithMultipleArrayParameters(
            const std::vector<std::string>& stringArrayArg,
            const std::vector<int8_t>& int8ArrayArg,
            const std::vector<joynr::interlanguagetest::namedTypeCollection2::
                                      ExtendedInterfaceEnumerationInTypeCollection::Enum>&
                    enumArrayArg,
            const std::vector<
                    joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                    structWithStringArrayArrayArg,
            std::function<void(
                    const std::vector<uint64_t>& uInt64ArrayOut,
                    const std::vector<
                            joynr::interlanguagetest::namedTypeCollection1::StructWithStringArray>&
                            structWithStringArrayArrayOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithSingleEnumParameters(
            const joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedEnumerationWithPartlyDefinedValues::Enum& enumerationArg,
            std::function<void(const joynr::interlanguagetest::namedTypeCollection2::
                                       ExtendedTypeCollectionEnumerationInTypeCollection::Enum&
                                               enumerationOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithMultipleEnumParameters(
            const joynr::interlanguagetest::Enumeration::Enum& enumerationArg,
            const joynr::interlanguagetest::namedTypeCollection2::
                    ExtendedTypeCollectionEnumerationInTypeCollection::Enum& extendedEnumerationArg,
            std::function<void(const joynr::interlanguagetest::namedTypeCollection2::
                                       ExtendedEnumerationWithPartlyDefinedValues::Enum&
                                               extendedEnumerationOut,
                               const joynr::interlanguagetest::Enumeration::Enum& enumerationOut)>
                    onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithSingleStructParameters(
            const joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct&
                    extendedBaseStructArg,
            std::function<void(const joynr::interlanguagetest::namedTypeCollection2::
                                       ExtendedStructOfPrimitives& extendedStructOfPrimitivesOut)>
                    onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithMultipleStructParameters(
            const joynr::interlanguagetest::namedTypeCollection2::ExtendedStructOfPrimitives&
                    extendedStructOfPrimitivesArg,
            const joynr::interlanguagetest::namedTypeCollection2::BaseStruct& baseStructArg,
            std::function<void(
                    const joynr::interlanguagetest::namedTypeCollection2::BaseStructWithoutElements&
                            baseStructWithoutElementsOut,
                    const joynr::interlanguagetest::namedTypeCollection2::
                            ExtendedExtendedBaseStruct& extendedExtendedBaseStructOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    void methodFireAndForgetWithoutParameter() override;

    void methodFireAndForgetWithInputParameter(const std::int32_t& int32Arg) override;

    virtual void overloadedMethod(
            std::function<void(const std::string& stringOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void overloadedMethod(
            const bool& booleanArg,
            std::function<void(const std::string& stringOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void overloadedMethod(
            const std::vector<joynr::interlanguagetest::namedTypeCollection2::
                                      ExtendedExtendedEnumeration::Enum>& enumArrayArg,
            const int64_t& int64Arg,
            const joynr::interlanguagetest::namedTypeCollection2::BaseStruct& baseStructArg,
            const bool& booleanArg,
            std::function<
                    void(const double& doubleOut,
                         const std::vector<std::string>& stringArrayOut,
                         const joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct&
                                 extendedBaseStructOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void overloadedMethodWithSelector(
            std::function<void(const std::string& stringOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void overloadedMethodWithSelector(
            const bool& booleanArg,
            std::function<void(const std::string& stringOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void overloadedMethodWithSelector(
            const std::vector<joynr::interlanguagetest::namedTypeCollection2::
                                      ExtendedExtendedEnumeration::Enum>& enumArrayArg,
            const int64_t& int64Arg,
            const joynr::interlanguagetest::namedTypeCollection2::BaseStruct& baseStructArg,
            const bool& booleanArg,
            std::function<
                    void(const double& doubleOut,
                         const std::vector<std::string>& stringArrayOut,
                         const joynr::interlanguagetest::namedTypeCollection2::ExtendedBaseStruct&
                                 extendedBaseStructOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithStringsAndSpecifiedStringOutLength(
            const std::string& stringArg,
            const int32_t& int32StringLengthArg,
            std::function<void(const std::string& stringOut)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithoutErrorEnum(
            const std::string& wantedExceptionArg,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

    virtual void methodWithAnonymousErrorEnum(
            const std::string& wantedExceptionArg,
            std::function<void()> onSuccess,
            std::function<void(const joynr::interlanguagetest::TestInterface::
                                       MethodWithAnonymousErrorEnumErrorEnum::Enum& errorEnum)>
                    onError) override;

    virtual void methodWithExistingErrorEnum(
            const std::string& wantedExceptionArg,
            std::function<void()> onSuccess,
            std::function<void(const joynr::interlanguagetest::namedTypeCollection2::
                                       ExtendedErrorEnumTc::Enum& errorEnum)> onError) override;

    virtual void methodWithExtendedErrorEnum(
            const std::string& wantedExceptionArg,
            std::function<void()> onSuccess,
            std::function<void(const joynr::interlanguagetest::TestInterface::
                                       MethodWithExtendedErrorEnumErrorEnum::Enum& errorEnum)>
                    onError) override;

    virtual void methodToFireBroadcastWithSinglePrimitiveParameter(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodToFireBroadcastWithMultiplePrimitiveParameters(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodToFireBroadcastWithSingleArrayParameter(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodToFireBroadcastWithMultipleArrayParameters(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodToFireBroadcastWithSingleEnumerationParameter(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodToFireBroadcastWithMultipleEnumerationParameters(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodToFireBroadcastWithSingleStructParameter(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodToFireBroadcastWithMultipleStructParameters(
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodToFireBroadcastWithFiltering(
            const std::string& stringArg,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

    void getAttributeFireAndForget(
            std::function<void(const std::int32_t&)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

    void setAttributeFireAndForget(
            const std::int32_t& attributeFireAndForget,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

    virtual void getAttributeWithExceptionFromGetter(
            std::function<void(const bool&)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void getAttributeWithExceptionFromSetter(
            std::function<void(const bool&)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void setAttributeWithExceptionFromSetter(
            const bool& attributeWithExceptionFromSetter,
            std::function<void()> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void getAttributeInt8readonlyNoSubscriptions(
            std::function<void(const int8_t&)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void getAttributeBooleanReadonly(
            std::function<void(const bool&)> onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void getAttributeExtendedEnumerationReadonly(
            std::function<void(const joynr::interlanguagetest::namedTypeCollection2::
                                       ExtendedEnumerationWithPartlyDefinedValues::Enum&)>
                    onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException& exception)>
                    onError) override;

    virtual void methodWithSingleMapParameters(
            const joynr::interlanguagetest::namedTypeCollection2::MapStringString& mapArg,
            std::function<void(
                    const joynr::interlanguagetest::namedTypeCollection2::MapStringString& mapOut)>
                    onSuccess,
            std::function<void(const joynr::exceptions::ProviderRuntimeException&)> onError)
            override;

private:
    // Disallow copy and assign
    IltProvider(const IltProvider&);
    void operator=(const IltProvider&);

    std::mutex mutex; // Providers need to be threadsafe

    std::int32_t attributeFireAndForget;

    ADD_LOGGER(IltProvider);
};

#endif
