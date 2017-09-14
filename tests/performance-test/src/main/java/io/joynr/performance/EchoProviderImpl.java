package io.joynr.performance;

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

import io.joynr.provider.Promise;
import joynr.tests.performance.EchoAbstractProvider;
import joynr.tests.performance.Types.ComplexStruct;
import io.joynr.provider.Deferred;
import io.joynr.provider.DeferredVoid;

public class EchoProviderImpl extends EchoAbstractProvider {

    @Override
    public Promise<EchoStringDeferred> echoString(String data) {
        EchoStringDeferred deferred = new EchoStringDeferred();
        deferred.resolve(data);
        return new Promise<EchoStringDeferred>(deferred);
    }

    @Override
    public Promise<EchoByteArrayDeferred> echoByteArray(Byte[] data) {
        EchoByteArrayDeferred deferred = new EchoByteArrayDeferred();
        deferred.resolve(data);
        return new Promise<EchoByteArrayDeferred>(deferred);
    }

    @Override
    public Promise<EchoComplexStructDeferred> echoComplexStruct(ComplexStruct data) {
        EchoComplexStructDeferred deferred = new EchoComplexStructDeferred();
        deferred.resolve(data);
        return new Promise<EchoComplexStructDeferred>(deferred);
    }

    @Override
    public Promise<Deferred<String>> getSimpleAttribute() {
        // TODO Auto-generated method stub
        return null;
    }

    @Override
    public Promise<DeferredVoid> setSimpleAttribute(String simpleAttribute) {
        // TODO Auto-generated method stub
        return null;
    }
}
