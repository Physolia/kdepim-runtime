/*
    Copyright (c) 2007 Volker Krause <vkrause@kde.org>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_QUERYBUILDER_H
#define AKONADI_QUERYBUILDER_H

#include "storage/datastore.h"

namespace Akonadi {

/**
  Helper class for creating and executing database SELECT queries.
*/
template <typename T> class QueryBuilder
{
  public:
    /**
      Creates a new query builder.
    */
    inline QueryBuilder() : mQuery( DataStore::self()->database() ) {}

    /**
      Add a WHERE condition.
      @param column The column that should be compared.
      @param op The operator used for comparison
      @param value The value @p column is compared to.
    */
    inline void addCondition( const QString &column, const char* op, const QVariant &value )
    {
      Condition c;
      c.column = column;
      c.op = QString::fromLatin1( op );
      c.value = value;
      mConditions << c;
    }

    /**
      Executes the query, returns true on success.
    */
    inline bool exec()
    {
      QString statement = QLatin1String( "SELECT " );
      statement += T::columnNames().join( QLatin1String( ", " ) );
      statement += QLatin1String(" FROM ");
      statement += T::tableName();
      if ( !mConditions.isEmpty() ) {
        statement += QLatin1String(" WHERE ");
        int i = 0;
        QStringList conds;
        foreach ( const Condition c, mConditions ) {
          QString cstmt = c.column;
          cstmt += QLatin1Char( ' ' );
          cstmt += c.op;
          cstmt += QLatin1Char( ' ' );
          if ( c.value.isValid() )
            cstmt += QString::fromLatin1( ":%1" ).arg( i++ );
          else
            cstmt += QLatin1String( "NULL" );
          conds << cstmt;
        }
        statement += conds.join( QLatin1String( " AND " ) );
      }
      mQuery.prepare( statement );
      int i = 0;
      foreach ( const Condition c, mConditions )
        if ( c.value.isValid() )
          mQuery.bindValue( QString::fromLatin1( ":%1" ).arg( i++ ), c.value );
      if ( !mQuery.exec() ) {
        qDebug() << "Error during selecting records from table"
            << T::tableName() << mQuery.lastError().text();
        qDebug() << "Query was:" << statement;
        return false;
      }
      return true;
    }

    /**
      Returns the result of this SELECT query.
    */
    QList<T> result()
    {
      return T::extractResult( mQuery );
    }

  private:
    class Condition
    {
      public:
        QString column;
        QString op;
        QVariant value;
    };
    QList<Condition> mConditions;
    QSqlQuery mQuery;
};

}

#endif
