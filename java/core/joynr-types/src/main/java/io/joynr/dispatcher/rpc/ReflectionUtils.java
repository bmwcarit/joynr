package io.joynr.dispatcher.rpc;

/*
 * #%L
 * %%
 * Copyright (C) 2011 - 2013 BMW Car IT GmbH
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

import io.joynr.dispatcher.rpc.annotation.JoynrRpcReturn;

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;

import javax.annotation.CheckForNull;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Utility methods for the reflection part of Joyn-RPC
 */
public class ReflectionUtils {
    /**
     * Utility function to find a method in a class by name.
     * 
     * @param parameterTypes
     * 
     * @return any method in the class that has the specified method name
     */
    private static final Logger logger = LoggerFactory.getLogger(ReflectionUtils.class);

    // TODO findMethod looks for the method name only. =>Overloading is not supported. ParameterTypes should be checked
    // too. Workaround for sync/async methods with same name: only sync interface is used.
    @Deprecated
    public static Method findMethod(Class<?> clazz, String methodName) throws NoSuchMethodException {
        logger.warn("Warning findMethod uses only the method name to find a method to invoke => method overloading is not supported.");
        Method[] methods = clazz.getDeclaredMethods();
        for (Method method : methods) {
            if (method.getName().equals(methodName)) {
                return method;

            }
        }

        throw new NoSuchMethodException(methodName);
    }

    public static Method findMethodByParamTypes(Class<?> clazz, String methodName, Class<?>[] parameterTypes)
                                                                                                             throws NoSuchMethodException {
        Method[] methods = clazz.getDeclaredMethods();
        for (Method method : methods) {
            if (method.getName().equals(methodName)) {
                if (checkParameterTypes(method, parameterTypes)) {
                    return method;
                }
            }
        }

        throw new NoSuchMethodException(methodName);
    }

    private static boolean checkParameterTypes(Method method, Class<?>[] parameterTypes) {
        Class<?>[] currentMethodParamTypes = method.getParameterTypes();
        if (currentMethodParamTypes.length != parameterTypes.length) {
            return false;
        }
        for (int i = 0; i < parameterTypes.length; i++) {
            if (!currentMethodParamTypes[i].isAssignableFrom(parameterTypes[i])) {
                return false;
            }
        }
        return true;
    }

    public static Method findMethodByParamTypeNames(Class<?> clazz, String methodName, List<String> paramTypeNames)
                                                                                                                   throws NoSuchMethodException {
        Method[] methods = clazz.getDeclaredMethods();
        for (Method method : methods) {
            if (method.getName().equals(methodName)) {
                if (checkParameterTypeNames(method, paramTypeNames)) {
                    return method;
                }
            }
        }
        throw new NoSuchMethodException(methodName);
    }

    private static boolean checkParameterTypeNames(Method method, List<String> paramTypeNames) {
        if (paramTypeNames == null) {
            String msg = "Received RPC without parameter types list! Method overloading might cause unexpected behaviour!";
            logger.error(msg);
            throw new IllegalArgumentException(msg);
        }

        Class<?>[] currentMethodParamTypes = method.getParameterTypes();
        if (paramTypeNames.size() != currentMethodParamTypes.length) {
            return false;

        }
        for (int i = 0; i < currentMethodParamTypes.length; i++) {
            String currentParamName = currentMethodParamTypes[i].getName();
            String matchingParamName = paramTypeNames.get(i);
            if (!currentParamName.equals(matchingParamName)) {
                return false;
            }
        }
        return true;
    }

    @CheckForNull
    public static JoynrRpcReturn findReturnAnnotation(Method method) {
        JoynrRpcReturn res = method.getAnnotation(JoynrRpcReturn.class);
        if (res != null) {
            return res;
        }

        for (Class<?> interfaceClass : method.getDeclaringClass().getInterfaces()) {
            try {
                if (JoynrSyncInterface.class.isAssignableFrom(interfaceClass)) {

                    res = findReturnAnnotation(findMethodByParamTypes(interfaceClass,
                                                                      method.getName(),
                                                                      method.getParameterTypes()));
                    if (res != null) {
                        return res;
                    }
                }
            } catch (NoSuchMethodException e) {
            }
        }

        if (method.getDeclaringClass().getSuperclass() != null) {
            try {
                res = findReturnAnnotation(findMethodByParamTypes(method.getDeclaringClass().getSuperclass(),
                                                                  method.getName(),
                                                                  method.getParameterTypes()));
                if (res != null) {
                    return res;
                }
            } catch (NoSuchMethodException e) {
            }
        }

        return null;
    }

    /**
     * Utility function to find all annotations on the parameters of the specified method and merge them with the same
     * annotations on all base classes and interfaces.
     */
    public static List<List<Annotation>> findAndMergeParameterAnnotations(Method method) {
        List<List<Annotation>> res = new ArrayList<List<Annotation>>(method.getParameterTypes().length);
        for (int i = 0; i < method.getParameterTypes().length; i++) {
            res.add(new LinkedList<Annotation>());
        }
        findAndMergeAnnotations(method.getDeclaringClass(), method, res);
        return res;
    }

    private static void findAndMergeAnnotations(Class<?> clazz, Method method, List<List<Annotation>> res) {
        Method[] methods = clazz.getDeclaredMethods();
        for (Method currentMethod : methods) {
            if (areMethodNameAndParameterTypesEqual(currentMethod, method)) {
                for (int i = 0; i < currentMethod.getParameterAnnotations().length; i++) {
                    for (Annotation annotation : currentMethod.getParameterAnnotations()[i]) {
                        res.get(i).add(annotation);
                    }
                }
            }
        }

        for (Class<?> interfaceClass : clazz.getInterfaces()) {
            findAndMergeAnnotations(interfaceClass, method, res);
        }

        if (clazz.getSuperclass() != null) {
            findAndMergeAnnotations(clazz.getSuperclass(), method, res);
        }
    }

    /**
     * Compares to methods for equality based on name and parameter types.
     */
    private static boolean areMethodNameAndParameterTypesEqual(Method methodA, Method methodB) {
        if (!methodA.getName().equals(methodB.getName())) {
            return false;
        }
        Class<?>[] methodAParameterTypes = methodA.getParameterTypes();
        Class<?>[] methodBParameterTypes = methodB.getParameterTypes();
        if (methodAParameterTypes.length != methodBParameterTypes.length) {
            return false;
        }
        for (int i = 0; i < methodAParameterTypes.length; i++) {
            if (!methodAParameterTypes[i].equals(methodBParameterTypes[i])) {
                return false;
            }
        }

        return true;
    }
}
