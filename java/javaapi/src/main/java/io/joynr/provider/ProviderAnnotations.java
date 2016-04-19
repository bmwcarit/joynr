package io.joynr.provider;

/*
 * #%L
 * %%
 * Copyright (C) 2016 BMW Car IT GmbH
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

import io.joynr.exceptions.JoynrRuntimeException;
import io.joynr.util.AnnotationUtil;

import java.lang.annotation.Annotation;

public class ProviderAnnotations {

    public static String getInterfaceName(JoynrProvider provider) throws JoynrRuntimeException {
        return getAnnotation(provider.getClass(), InterfaceName.class).value();
    }

    public static Class<? extends JoynrProvider> getProvidedInterface(JoynrProvider provider)
                                                                                             throws JoynrRuntimeException {
        return getAnnotation(provider.getClass(), InterfaceClass.class).value();
    }

    public static int getMajorVersion(JoynrProvider provider) throws JoynrRuntimeException {
        return getAnnotation(provider.getClass(), MajorVersion.class).value();
    }

    public static int getMinorVersion(JoynrProvider provider) throws JoynrRuntimeException {
        return getAnnotation(provider.getClass(), MinorVersion.class).value();
    }

    private static <T extends Annotation> T getAnnotation(Class<?> clazz, Class<T> annotationType)
                                                                                                  throws JoynrRuntimeException {
        T annotation = AnnotationUtil.getAnnotation(clazz, annotationType);
        if (annotation == null) {
            throw new JoynrRuntimeException("Missing annotation " + annotationType.getName()
                    + " in hierarchy of class " + clazz.getName());
        }
        return annotation;
    }

}
