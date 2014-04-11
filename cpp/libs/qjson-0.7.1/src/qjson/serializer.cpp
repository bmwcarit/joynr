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

#include "serializer.h"

#include <QtCore/QDataStream>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtCore/QMetaObject>
#include <QtCore/QMetaProperty>
#include <QtCore/QMap>
#include <QtCore/QMetaEnum>
#include <QtCore/QMutex>
#include <iostream>
using namespace QJson;

QMutex* Serializer::enumIdsMutex = new QMutex();
QMap<int, QMetaEnum>* Serializer::enumIds = new QMap<int,QMetaEnum>();

class Serializer::SerializerPrivate {
};

Serializer::Serializer() : d( new SerializerPrivate ) {
}

Serializer::~Serializer() {
  delete d;
}

void Serializer::registerEnum( int typeId, const QMetaEnum& meta )
{
    QMutexLocker locker(enumIdsMutex);
    if (!enumIds->contains(typeId)) {
        enumIds->insert(typeId, meta);
    }
}

void Serializer::serialize( const QVariant& v, QIODevice* io, bool* ok )
{
  Q_ASSERT( io );
  if (!io->isOpen()) {
    if (!io->open(QIODevice::WriteOnly)) {
      if ( ok != 0 )
        *ok = false;
      qCritical ("Error opening device");
      return;
    }
  }

  if (!io->isWritable()) {
    if (ok != 0)
      *ok = false;
    qCritical ("Device is not readable");
    io->close();
    return;
  }

  const QByteArray str = serialize( v );
  if ( !str.isNull() ) {
    QDataStream stream( io );
    stream << str;
  } else {
    if ( ok )
      *ok = false;
  }
}

// Delimits special characters in a string and surrounds the string in quotes
// Returns a QByteArray in UTF-8 format
static QByteArray sanitizeString( const QString& str )
{
  QByteArray ret;
  const QChar *data = str.constData();

  // Add the first quote
  ret.append('"');

  // Delimit special characters
  int length = str.length();

  for (int i = 0; i < length; i++) {
     char ch = data[i].toLatin1();
     switch(ch) {
     case '\b' :
         ret.append("\\b");
         break;
     case '\f' :
         ret.append("\\f");
         break;
     case '\n' :
         ret.append("\\n");
         break;
     case '\r' :
         ret.append("\\r");
         break;
     case '\t' :
         ret.append("\\t");
         break;
     case '\\' :
     case '\"' :
         ret.append('\\');
         ret.append(ch);
         break;
     default:
         ret.append(QString(data[i]).toUtf8());
     }
  }

  // Add the final quote
  ret.append('"');
  return ret;
}

// Delimits special characters in a byte array and surrounds the string in quotes
// Returns a QByteArray in UTF-8 format
static QByteArray sanitizeByteArray( const QByteArray& array )
{
  QByteArray ret;
  const char *data = array.constData();

  // Add the first quote
  ret.append('"');

  // Delimit special characters
  int length = array.length();

  for (int i = 0; i < length; i++) {
     char ch = data[i];
     switch(ch) {
     case '\b' :
         ret.append("\\b");
         break;
     case '\f' :
         ret.append("\\f");
         break;
     case '\n' :
         ret.append("\\n");
         break;
     case '\r' :
         ret.append("\\r");
         break;
     case '\t' :
         ret.append("\\t");
         break;
     case '\\' :
     case '\"' :
         ret.append('\\');
         ret.append(ch);
         break;
     default:
         ret.append(ch);
     }
  }

  // Add the final quote
  ret.append('"');
  return ret;
}

QByteArray Serializer::serialize( const QVariant &v )
{
  QByteArray str;
  bool error = false;
  if ( ! v.isValid() ) { // invalid or null?
    str = "null";
  } else if ( v.type() == QVariant::List ) { // variant is a list?

    const QVariantList list = v.toList();
    str = "[";
    bool first = true;
    Q_FOREACH( const QVariant& v, list )
    {
      QByteArray serializedValue = serialize( v );
      if ( serializedValue.isNull() ) {
        error = true;
        break;
      }
      if (!first) {
          str.append(",");
      } else {
          first = false;
      }
      str.append(serializedValue);
    }
    str.append("]");
  } else if ( v.type() == QVariant::Map ) { // variant is a map?
    const QVariantMap vmap = v.toMap();
    QMapIterator<QString, QVariant> it( vmap );
    str = "{";
    bool first = true;
    while ( it.hasNext() ) {
      it.next();
      QByteArray serializedValue = serialize( it.value() );
      if ( serializedValue.isNull() ) {
        error = true;
        break;
      }
      if (!first) {
          str.append(",");
      } else {
          first = false;
      }
      str.append(sanitizeString(it.key()));
      str.append(":");
      str.append(serializedValue);
    }
    str.append("}");
  } else if ( v.type() == QVariant::String ) { // a string
    str = sanitizeString( v.toString() );
  } else if ( v.type() == QVariant::ByteArray ) { // a byte array?
    str = sanitizeByteArray( v.toByteArray() );
  } else if ( v.type() == QVariant::Double ) { // a double?
      str = QByteArray::number( v.toDouble(), 'g', 15 );
    if( ! str.contains( "." ) && ! str.contains( "e" ) ) {
      str += ".0";
    }
  } else if ( v.type() == QVariant::Bool ) { // boolean value?
    str = ( v.toBool() ? "true" : "false" );
// remember the current state of GCC diagnostics
#pragma GCC diagnostic push
// disable GCC enum-compare warning:
// Although v.type() is declared as returning QVariant::Type, the return value
// should be interpreted as QMetaType::Type (cf. http://qt-project.org/doc/qt-5/qvariant.html#type).
#pragma GCC diagnostic ignored "-Wenum-compare"
  } else if ( v.type() == QMetaType::UChar        // quint8, unsigned char
              || v.type() == QMetaType::UShort    // quint16, unsigned short
              || v.type() == QMetaType::UInt      // quint32, unsigned int
              || v.type() == QMetaType::ULong
              || v.type() == QMetaType::ULongLong // quint64, unsigned long long
  ) {
// restore previous state of GCC diagnostics
#pragma GCC diagnostic pop
    str = QByteArray::number( v.value<qulonglong>() );
// remember the current state of GCC diagnostics
#pragma GCC diagnostic push
// disable GCC enum-compare warning:
// Although v.type() is declared as returning QVariant::Type, the return value
// should be interpreted as QMetaType::Type (cf. http://qt-project.org/doc/qt-5/qvariant.html#type).
#pragma GCC diagnostic ignored "-Wenum-compare"
  } else if ( v.type() == QMetaType::SChar       // qint8, (signed) char
              || v.type() == QMetaType::Char
              || v.type() == QMetaType::Short    // qint16, (signed) short
              || v.type() == QMetaType::Int      // qint32, (signed) int
              || v.type() == QMetaType::Long
              || v.type() == QMetaType::LongLong // qint64, (signed) long long
  ) {
// restore previous state of GCC diagnostics
#pragma GCC diagnostic pop
    str = QByteArray::number( v.value<qlonglong>() );
  } else if (v.type() == QVariant::UserType) {
    int typeId = v.userType();
    // Does the variant contain a user defined enum?
    if (enumIds->contains(typeId)) {
        // enum
        QMetaEnum metaEnum = enumIds->value(typeId);

        // Convert the enum value to a key
        const int *pvalue = static_cast<const int *>(v.constData());
        QString key = QLatin1String(metaEnum.valueToKey(*pvalue));
        str = sanitizeString(key);
    } else {
        // QObject
        //If this point is reached, we know it came from a QObject of some sort
        const QObject *obj  = static_cast<const QObject*>(v.constData());
        QVariantMap result;
        const QMetaObject *metaobject = obj->metaObject();
        QVariant typeNameVariant(turnMetaObjectIntoTypename(metaobject));
        result[QString::fromLatin1("_typeName")] = typeNameVariant;
        int count = metaobject->propertyCount();
        for (int i = 0; i < count; ++i) {
            QMetaProperty metaproperty = metaobject->property(i);
            const char *name = metaproperty.name();
            // const char *typeName = metaproperty.typeName();
            if (!metaproperty.isReadable() || !strcmp(name, "objectName")) {
                continue;
            }
            QVariant value = obj->property(name);
            result[QLatin1String(name)] = value;

            // The line below sometimes fails with VisualStudio, see Joynr-1121
            // std::cerr << "name: " << name << ", type: " << value.typeName() << std::endl;
//            if (!strcmp(typeName, "QVariant")){// && value.type() == QVariant::UserType){
//              const QObject *obj2  = static_cast<const QObject*>(value.constData());
//              const QMetaObject *metaobject2 = obj2->metaObject();
//              QVariant typeNameVariant(QString(metaobject2->className()));
//              std::cout << metaobject2->className() << std::endl;
//              result[QLatin1String(name)+ QLatin1String("_typeName")] = typeNameVariant;
//            }

        }
        str = serialize( result );
    }
  } else if ( v.canConvert<QString>() ){ // Last chance: can value be converted to string?
    // this will catch QDate, QDateTime, QUrl, ...
    str = sanitizeString( v.toString() );
    //TODO: catch other values like QImage, QRect, ...
  } else {
    // unable to serialize QVariant
    // TODO: error handling
    error = true;
  }
  if ( !error )
    return str;
  else
    return QByteArray();
}

QString Serializer::turnMetaObjectIntoTypename(const QMetaObject* metaObject){
    static QString dot = QString::fromLatin1(".");
    static QString doubleColon = QString::fromLatin1("::");

    QString fullClassName = QString::fromLatin1(metaObject->className());

    return fullClassName.replace(doubleColon, dot);
}
