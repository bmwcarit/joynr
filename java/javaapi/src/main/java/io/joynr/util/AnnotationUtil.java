package io.joynr.util;

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

import java.lang.annotation.Annotation;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;

import com.google.common.collect.Iterables;

public class AnnotationUtil {

    public static Collection<Annotation> getAnnotationsRecursive(Class<?> clazz) {
        Collection<Annotation> allAnnotations = new HashSet<>();
        getAllAnnotations(clazz, allAnnotations);
        return allAnnotations;
    }

    public static <T extends Annotation> T getAnnotation(Class<?> clazz, Class<T> annotationType) {
        Iterable<T> allAnnotations = Iterables.filter(getAnnotationsRecursive(clazz), annotationType);
        if (allAnnotations.iterator().hasNext()) {
            return allAnnotations.iterator().next();
        }
        return null;
    }

    private static void getAllAnnotations(Class<?> clazz, Collection<Annotation> result) {
        if (clazz == null) {
            return;
        }
        result.addAll(Arrays.asList(clazz.getDeclaredAnnotations()));
        for (Class<?> interfaceClass : clazz.getInterfaces()) {
            getAllAnnotations(interfaceClass, result);
        }

        getAllAnnotations(clazz.getSuperclass(), result);
    }
}
