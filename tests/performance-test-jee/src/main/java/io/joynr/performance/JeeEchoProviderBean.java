package io.joynr.performance;

import javax.ejb.Stateless;

import io.joynr.jeeintegration.api.ProviderDomain;
import io.joynr.jeeintegration.api.ServiceProvider;
import joynr.tests.performance.EchoSync;
import joynr.tests.performance.Types.ComplexStruct;

@Stateless
@ServiceProvider(serviceInterface = EchoSync.class)
@ProviderDomain("performance_test_domain")
public class JeeEchoProviderBean implements EchoSync {

    @Override
    public String echoString(String data) {
        return data;
    }

    @Override
    public Byte[] echoByteArray(Byte[] data) {
        return data;
    }

    @Override
    public ComplexStruct echoComplexStruct(ComplexStruct data) {
        return data;
    }
}
