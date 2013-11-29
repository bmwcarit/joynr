/* This file is part of qjson
  *
  * Copyright (C) 2009 Till Adam <adam@kde.org>
  * Copyright (C) 2009 Flavio Castelli <flavio@castelli.name>
  * Copyright (C) 2013 BMW Car IT GmbH
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "qobjecthelper.h"
#include <QString>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QObject>
#include <cassert>

#include <iostream>

using namespace QJson;

QString QObjectHelper::typenameKey = QString::fromLatin1("_typeName");

class QObjectHelper::QObjectHelperPrivate {
};

QObjectHelper::QObjectHelper()
  : d (new QObjectHelperPrivate)
{
}

QObjectHelper::~QObjectHelper()
{
  delete d;
}

QVariantMap QObjectHelper::qobject2qvariant(
        const QObject* object,
        const QStringList& ignoredProperties)
{
  QVariantMap result;
  const QMetaObject *metaobject = object->metaObject();
  int count = metaobject->propertyCount();
  for (int i=0; i<count; ++i) {
    QMetaProperty metaproperty = metaobject->property(i);
    const char *name = metaproperty.name();

    if (ignoredProperties.contains(QLatin1String(name)) || (!metaproperty.isReadable()))
      continue;
    QVariant value = object->property(name);
    result[QLatin1String(name)] = value;
 }
  return result;
}

void QObjectHelper::transformElementsOfQVariantList(QVariantList& variantList){

    // Loop through the variant list
    QList<QVariant>::iterator iter = variantList.begin();
    while (iter != variantList.end()) {
        // Transform QVariantMap entries into user defined objects
        if ((*iter).canConvert<QVariantMap>()){
            QVariantMap variantMap = (*iter).value<QVariantMap>();
            if (variantMap.contains(typenameKey)){
                QString typeName = variantMap.take(typenameKey).value<QString>();
                int classId = getClassIdForTransmittedType(typeName);
                assertClassId(classId, typeName);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                void* object = QMetaType::construct(classId);
#else
                void* object = QMetaType::create(classId);
#endif
                qvariant2qobject(variantMap,(QObject*)object);

                *iter = QVariant(classId, object);
                QMetaType::destroy(classId, object);
            }
        }
        // Transform QVariantList entries into user defined objects
        else if ((*iter).canConvert<QVariantList>()){
            QVariantList oldInternalVariantList = (*iter).value<QVariantList>();
            transformElementsOfQVariantList(oldInternalVariantList);
            *iter = oldInternalVariantList;
        }
        iter++;
    }
}

void QObjectHelper::transformElementsOfQVariantMap(QVariantMap& variantMapOld, QVariantMap& variantMapNew){
    QVariantMap::const_iterator iter = variantMapOld.begin();
    while(iter != variantMapOld.end()){
       if (iter.value().canConvert<QVariantMap>()){
            QVariantMap variantMap = iter.value().value<QVariantMap>();
            if (variantMap.contains(typenameKey)) {
                QString typeName = variantMap.take(typenameKey).value<QString>();
                int classId = getClassIdForTransmittedType(typeName);
                assertClassId(classId, typeName);
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                void* object = QMetaType::construct(classId);
#else
                void* object = QMetaType::create(classId);
#endif
                qvariant2qobject(variantMap,(QObject*)object);
                variantMapNew.insert(iter.key(), QVariant(classId, object));
                QMetaType::destroy(classId, object);
            }
            else{
                variantMapNew.insert(iter.key(), iter.value());
            }
        }
        else if (iter.value().canConvert<QVariantList>()) {
            QVariantList variantList = iter.value().value<QVariantList>();
            transformElementsOfQVariantList(variantList);
            variantMapNew.insert(iter.key(), variantList);
        }
        else{
            variantMapNew.insert(iter.key(), iter.value());
        }
        iter++;
    }
}

void QObjectHelper::qvariant2qobject(const QVariantMap& variant, QObject* object)
{
    assert(object);
    QStringList properties;
    QStringList enumTypes;
    QMap<QString, QString> propertyType;
    const QMetaObject *metaobject = object->metaObject();
    int count = metaobject->propertyCount();

    for (int i=0; i<count; ++i) {
      QMetaProperty metaproperty = metaobject->property(i);
      QString propertyName = QString::fromLatin1(metaproperty.name());

      if (metaproperty.isWritable()) {
        properties.append(propertyName);
        propertyType[propertyName] = QString::fromLatin1(metaproperty.typeName());
      }
      if (metaproperty.isEnumType()) {
        enumTypes << propertyName;
      }
    }

    QVariantMap::const_iterator iter;
    QByteArray propertyName;

    for (iter = variant.constBegin(); iter != variant.end(); iter++) {
      propertyName = iter.key().toLatin1();

      if (properties.contains(iter.key())) {
          if (enumTypes.contains(iter.key())){
              if (iter.value().canConvert<qulonglong>()){
                  QVariant value;
                  value.setValue((int)iter.value().value<qulonglong>());
                  object->setProperty(propertyName, value);
              }
              else{
                  //TODO ca: other work around required in this case?
                  object->setProperty(propertyName, iter.value());
              }
          }
          else if (iter.value().canConvert<QVariantList>()){

               QVariantList variantList = iter.value().value<QVariantList>();
               transformElementsOfQVariantList(variantList);
               if ( object->setProperty(propertyName, variantList) == false ) {
                   qWarning("Cannot add property %s", propertyName.constData());
               }
          }
          else if (iter.value().canConvert<QVariantMap>()){
              QVariantMap variantMap = iter.value().value<QVariantMap>();
              QString typeName = propertyType[iter.key()];
              bool isQVariantWithUserType = !typeName.compare(QString::fromLatin1("QVariant")) && variantMap.contains(typenameKey);
              int classId = QMetaType::type(typeName.toLatin1().constData());

              if (!typeName.compare(QString::fromLatin1("QVariantMap"))){
                  QVariantMap variantMapNew;
                  transformElementsOfQVariantMap(variantMap, variantMapNew);
                  if ( object->setProperty(propertyName, variantMapNew) == false ) {
                      qWarning("Cannot add property %s", propertyName.constData());
                  }
              }
              else if (!isQVariantWithUserType && (classId == 0 || !typeName.compare(QString::fromLatin1("QVariant")))){
                  //one possible solution: qvariant with primitive objects
                  if ( object->setProperty(propertyName, iter.value()) == false ) {
                      qWarning("Cannot add property %s", propertyName.constData());
                  }
              } else {
                  if (isQVariantWithUserType){
                      // classId can be 0 indicating the type is not known, handle this gracefully.
                      QString userTypeName = variantMap.take(typenameKey).value<QString>();
                      classId = getClassIdForTransmittedType(userTypeName);
                      assertClassId(classId, userTypeName);
                  }
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
                  void* reconstructedObj = QMetaType::construct(classId);
#else
                  void* reconstructedObj = QMetaType::create(classId);
#endif
                  QJson::QObjectHelper::qvariant2qobject(variantMap,(QObject*) reconstructedObj);
                  QVariant variant = QVariant(classId, reconstructedObj);
                  QMetaType::destroy(classId, reconstructedObj);
                  if ( object->setProperty(propertyName, variant) == false ) {
                    qWarning("Cannot add property %s", propertyName.constData());
                  }
              }
          }
          else {
              if ( object->setProperty(propertyName, iter.value()) == false ) {
                  qWarning("Cannot add property %s", propertyName.constData());
              }
          }
      }
    }
}

int QObjectHelper::getClassIdForTransmittedType(const QString& typeName){
    QByteArray typeNameBytes = typeName.toLatin1();
    typeNameBytes.replace(".", QByteArray("::"));

    return QMetaType::type(typeNameBytes.constData());
}

void QObjectHelper::assertClassId(int classId, const QString& typeName)
{
    if (classId == 0) {
        qFatal("***** [] FATAL MSG - Cannot get classId for %s. Has the Metatype been registered?",
            typeName.toLatin1().constData());
    }
}
