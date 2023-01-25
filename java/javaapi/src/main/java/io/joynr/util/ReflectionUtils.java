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
package io.joynr.util;

import java.lang.annotation.Annotation;
import java.lang.reflect.Array;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

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
    private static final String BYTE = "Byte";
    private static final String SHORT = "Short";
    private static final String INTEGER = "Integer";
    private static final String LONG = "Long";
    private static final String FLOAT = "Float";
    private static final String DOUBLE = "Double";
    private static final String STRING = "String";
    private static final String BOOLEAN = "Boolean";

    public static List<Method> findMethodsByName(Class<?> clazz, String methodName) throws NoSuchMethodException {
        ArrayList<Method> methodsList = new ArrayList<Method>(Arrays.asList(clazz.getDeclaredMethods()));
        if (methodsList.isEmpty()) {
            throw new NoSuchMethodException(methodName);
        }

        for (Iterator<Method> iterator = methodsList.iterator(); iterator.hasNext();) {
            Method method = iterator.next();
            if (!method.getName().equals(methodName)) {
                iterator.remove();

            }
        }
        return methodsList;
    }

    public static Method findMethodByParamTypes(Class<?> clazz,
                                                String methodName,
                                                Class<?>[] parameterTypes) throws NoSuchMethodException {
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

    public static Method findMethodByParamTypeNames(Class<?> clazz,
                                                    String methodName,
                                                    List<String> paramTypeNames) throws NoSuchMethodException {
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

        String[] currentMethodParamTypes = toDatatypeNames(method.getParameterTypes());
        if (currentMethodParamTypes == null || (paramTypeNames.size() != currentMethodParamTypes.length)) {
            return false;

        }
        for (int i = 0; i < currentMethodParamTypes.length; i++) {
            String currentParamName = currentMethodParamTypes[i];
            String matchingParamName = paramTypeNames.get(i);
            if (!currentParamName.equals(matchingParamName)) {
                return false;
            }
        }
        return true;
    }

    /**
     * Utility function to find all annotations on the parameters of the specified method and merge them with the same
     * annotations on all base classes and interfaces.
     *
     * @param method the method in question
     * @return the list of annotations for this method
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

    public static String[] toDatatypeNames(Class<?>... types) {
        if (types == null) {
            return null;
        }

        String[] strings = new String[types.length];
        for (int i = 0; i < types.length; i++) {
            Class<?> type = types[i];
            if (type == null) {
                continue;
            } else {
                strings[i] = type.getCanonicalName().replace("java.lang.", "");
            }
        }
        return strings;
    }

    public static Class<?>[] toJavaClasses(String... typeNames) {
        if (typeNames == null) {
            return null;
        }

        Class<?>[] classes = new Class[typeNames.length];
        for (int i = 0; i < typeNames.length; i++) {
            String[] nameTokens = typeNames[i].split("\\[");
            Class<?> clazz;
            switch (nameTokens[0]) {
            case BOOLEAN:
                clazz = Boolean.class;
                break;
            case BYTE:
                clazz = Byte.class;
                break;
            case SHORT:
                clazz = Short.class;
                break;
            case INTEGER:
                clazz = Integer.class;
                break;
            case LONG:
                clazz = Long.class;
                break;
            case FLOAT:
                clazz = Float.class;
                break;
            case DOUBLE:
                clazz = Double.class;
                break;
            case STRING:
                clazz = String.class;
                break;
            default:
                try {
                    clazz = Class.forName(nameTokens[0]);
                } catch (ClassNotFoundException e) {
                    logger.debug("Class not found", e);
                    clazz = Object.class;
                }
                break;
            }
            clazz = processArrayTokens(clazz, nameTokens.length - 1);
            classes[i] = clazz;
        }
        return classes;
    }

    public static Method getStaticMethodFromSuperInterfaces(final Class<?> clazz,
                                                            String methodName) throws NoSuchMethodException {
        try {
            return clazz.getMethod(methodName);
        } catch (NoSuchMethodException e) {
            Class<?>[] parentClasses = clazz.getInterfaces();
            if (parentClasses.length == 0) {
                throw e;
            }
            for (Class<?> parent : parentClasses) {
                try {
                    return getStaticMethodFromSuperInterfaces(parent, methodName);
                } catch (NoSuchMethodException e2) {
                    // ignore
                }
            }
            throw e;
        }
    }

    private static Class<?> processArrayTokens(Class<?> clazz, int i) {
        if (i == 0) {
            return clazz;
        }
        return processArrayTokens(Array.newInstance(clazz, 0).getClass(), --i);
    }
}
