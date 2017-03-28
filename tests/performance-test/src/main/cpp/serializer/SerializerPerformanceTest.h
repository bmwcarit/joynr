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

#include <vector>
#include <numeric>
#include <string>

#include <boost/type_index.hpp>

#include "../common/PerformanceTest.h"
#include "joynr/Request.h"
#include "joynr/JoynrMessageFactory.h"
#include "joynr/MessagingQos.h"
#include "joynr/JoynrMessage.h"

#include "joynr/tests/performance/Types/ComplexStruct.h"
#include "joynr/serializer/Serializer.h"

template <typename Generator>
class SerializerPerformanceTest : public PerformanceTest
{
    using OutputStream = muesli::StringOStream;
    using OutputArchive = muesli::JsonOutputArchive<OutputStream>;
    using InputStream = muesli::StringIStream;
    using InputArchive = muesli::JsonInputArchive<InputStream>;

public:
    SerializerPerformanceTest(std::uint64_t runs, std::size_t length)
            : runs(runs),
              length(length),
              request(Generator::generateRequest(length)),
              messageFactory(),
              qos()
    {
    }

    void runSerializationBenchmark() const
    {
        auto fun = [this]() { this->createMessage(); };
        runAndPrintAverage(runs, getTestName("serialization"), fun);
    }

    void runFullMessageSerializationBenchmark() const
    {
        auto fun = [this]() {
            joynr::JoynrMessage message = this->createMessage();
            OutputStream ostream;
            OutputArchive oarchive(ostream);
            oarchive(message);
            return ostream.getString();
        };
        runAndPrintAverage(runs, getTestName("full message serialization"), fun);
    }

    template <typename ParamType>
    void runDeSerializationBenchmark() const
    {
        joynr::JoynrMessage message = createMessage();
        auto fun = [&message]() {
            joynr::Request deserializedRequest;
            InputStream istream(message.getPayload());
            auto iarchive = std::make_shared<InputArchive>(istream);
            (*iarchive)(deserializedRequest);
            ParamType param;
            deserializedRequest.getParams(param);
            return param;
        };

        runAndPrintAverage(runs, getTestName("deserialization"), fun);
    }

    template <typename ParamType>
    void runFullMessageDeSerializationBenchmark() const
    {
        // create a message, serialize it and then benchmark the deserialization
        joynr::JoynrMessage message = createMessage();

        OutputStream ostream;
        muesli::JsonOutputArchive<OutputStream> oarchive(ostream);
        oarchive(message);
        std::string serializedMessage = ostream.getString();

        auto fun = [&serializedMessage]() {
            InputStream msgStream(serializedMessage);
            joynr::JoynrMessage deserializedMessage;
            InputArchive messageInputArchive(msgStream);
            messageInputArchive(deserializedMessage);

            joynr::Request deserializedRequest;
            InputStream requestStream(deserializedMessage.getPayload());
            auto requestInputArchive = std::make_shared<InputArchive>(requestStream);
            (*requestInputArchive)(deserializedRequest);

            ParamType param;
            deserializedRequest.getParams(param);
            return param;
        };

        runAndPrintAverage(runs, getTestName("full message deserialization"), fun);
    }

private:
    joynr::JoynrMessage createMessage() const
    {
        OutputStream ostream;
        muesli::JsonOutputArchive<OutputStream> oarchive(ostream);
        oarchive(request);
        joynr::JoynrMessage msg;
        msg.setPayload(ostream.getString());
        return msg;
    }

    std::string getTestName(const std::string& testType) const
    {
        return testType + " " + boost::typeindex::type_id<Generator>().pretty_name() + " length=" +
               std::to_string(length);
    }

    std::uint64_t runs;
    std::size_t length;
    joynr::Request request;
    joynr::JoynrMessageFactory messageFactory;
    joynr::MessagingQos qos;

    const std::string senderParticipantId = "sender";
    const std::string receiverParticipantId = "receiver";
};

namespace generator
{

namespace helper
{
std::vector<std::int8_t> getFilledVector(std::size_t length)
{
    std::vector<std::int8_t> data(length);
    // fill data with sequentially increasing numbers
    std::iota(data.begin(), data.end(), 0);
    return data;
}

std::string getFilledString(std::size_t length)
{
    return std::string(length, '#');
}
} // namespace helper

struct ByteArray
{
    using type = std::vector<std::int8_t>;
    static joynr::Request generateRequest(std::size_t length)
    {
        joynr::Request request;
        request.setParams(helper::getFilledVector(length));
        request.setParamDatatypes({"Byte[]"});
        request.setMethodName("echoByteArray");
        return request;
    }
};

struct String
{
    using type = std::string;

    static joynr::Request generateRequest(std::size_t length)
    {
        joynr::Request request;
        request.setParams(helper::getFilledString(length));
        request.setParamDatatypes({"String"});
        request.setMethodName("echoString");
        return request;
    }
};

struct ComplexStruct
{
    using type = joynr::tests::performance::Types::ComplexStruct;

    static joynr::Request generateRequest(std::size_t length)
    {
        joynr::Request request;
        using joynr::tests::performance::Types::ComplexStruct;
        ComplexStruct complexStruct(
                32, 64, helper::getFilledVector(length), helper::getFilledString(length));
        request.setParams(complexStruct);
        request.setParamDatatypes({"joynr.tests.performance.Types.ComplexStruct"});
        request.setMethodName("echoComplexStruct");
        return request;
    }
};

} // namespace generator
