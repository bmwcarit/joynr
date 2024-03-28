/*-
 * #%L
 * %%
 * Copyright (C) 2011 - 2018 BMW Car IT GmbH
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
package io.joynr.jeeintegration;

import java.util.Set;
import java.util.function.Consumer;

import javax.ejb.Singleton;
import javax.enterprise.inject.spi.Bean;
import javax.enterprise.inject.spi.BeanManager;
import javax.enterprise.util.AnnotationLiteral;
import com.google.inject.Inject;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.joynr.UsedBy;
import io.joynr.jeeintegration.api.CallbackHandler;
import io.joynr.proxy.StatelessAsyncCallback;

@Singleton
public class CallbackHandlerDiscovery {

    private static final Logger logger = LoggerFactory.getLogger(CallbackHandlerDiscovery.class);

    @Inject
    private BeanManager beanManager;

    public void forEach(Consumer<StatelessAsyncCallback> consumer) {
        Set<Bean<?>> callbackHandlerBeans = beanManager.getBeans(Object.class,
                                                                 new AnnotationLiteral<CallbackHandler>() {
                                                                 });
        callbackHandlerBeans.forEach(callbackHandlerBean -> {
            for (Class<?> interfaceClass : callbackHandlerBean.getBeanClass().getInterfaces()) {
                if (StatelessAsyncCallback.class.isAssignableFrom(interfaceClass)) {
                    UsedBy usedBy = interfaceClass.getAnnotation(UsedBy.class);
                    if (usedBy == null) {
                        logger.warn("Stateless async callback interface " + interfaceClass + " implemented by "
                                + callbackHandlerBean
                                + " is not annotated with @UsedBy. Are you using an outdated version of the joynr "
                                + "code generator without setting the 'jee' parameter to ' true'? "
                                + "Note that the 'jee' property is now deprecated.");
                        continue;
                    }
                    StatelessAsyncCallback beanReference = (StatelessAsyncCallback) beanManager.getReference(callbackHandlerBean,
                                                                                                             interfaceClass,
                                                                                                             beanManager.createCreationalContext(callbackHandlerBean));
                    consumer.accept(beanReference);
                    break;
                }
            }
        });
    }

}
