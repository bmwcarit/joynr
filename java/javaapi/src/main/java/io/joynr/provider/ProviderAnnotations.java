package io.joynr.provider;

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

import io.joynr.JoynrVersion;
import io.joynr.util.AnnotationUtil;

import java.lang.annotation.Annotation;

public class ProviderAnnotations {

    public static String getInterfaceName(Class<?> providerClass) {
        return getAnnotation(providerClass, JoynrInterface.class).name();
    }

    public static String getInterfaceName(Object provider) {
        return getInterfaceName(provider.getClass());
    }

    public static Class<?> getProvidedInterface(Object provider) {
        return getAnnotation(provider.getClass(), JoynrInterface.class).provides();
    }

    public static int getMajorVersion(Object provider) {
        return getAnnotation(provider.getClass(), JoynrVersion.class).major();
    }

    public static int getMinorVersion(Object provider) {
        return getAnnotation(provider.getClass(), JoynrVersion.class).minor();
    }

    private static <T extends Annotation> T getAnnotation(Class<?> clazz, Class<T> annotationType) {
        T annotation = AnnotationUtil.getAnnotation(clazz, annotationType);
        if (annotation == null) {
            throw new IllegalArgumentException("Missing annotation " + annotationType.getName()
                    + " in hierarchy of class " + clazz.getName());
        }
        return annotation;
    }

}
