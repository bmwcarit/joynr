package io.joynr.logging;

import java.util.Properties;

import com.google.inject.Inject;
import com.google.inject.name.Named;

import io.joynr.messaging.MessagingPropertyKeys;
import io.joynr.runtime.JoynrRuntime;

public class JoynrAppenderManagerFactory {
    
    @Inject
    protected static JoynrRuntime runtime;
    
    @Inject
    @Named(MessagingPropertyKeys.JOYNR_PROPERTIES) Properties properties;

}
