/*
    This file is part of libkdepim.

    Copyright (c) 2003 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
#ifndef KCONFIGPROPAGATOR_H
#define KCONFIGPROPAGATOR_H

#include <qstring.h>
#include <qvaluelist.h>
#include <qdom.h>

class KConfigSkeleton;
class KConfigSkeletonItem;

class KConfigPropagator
{
  public:
    /**
      Create KConfigPropagator object.
      
      @param skeleton KConfigSkeleton object used as source for the propagation
      @param kcfgFile file name of kcfg file containing the propagation rules
    */
    KConfigPropagator( KConfigSkeleton *skeleton, const QString &kcfgFile );
    virtual ~KConfigPropagator() {}

    KConfigSkeleton *skeleton() { return mSkeleton; }

    /*
      Commit changes according to propagation rules.
    */
    void commit();

    class Condition
    {
      public:
        Condition() : isValid( false ) {}
      
        QString file;
        QString group;
        QString key;
        QString value;
        
        bool isValid;
    };

    class Rule
    {
      public:
        typedef QValueList<Rule> List;
        
        Rule() : hideValue( false ) {}
        
        QString sourceFile;
        QString sourceGroup;
        QString sourceEntry;

        QString targetFile;
        QString targetGroup;
        QString targetEntry;

        Condition condition;

        bool hideValue;
    };

    class Change
    {
      public:
        typedef QValueList<Change> List;

        Change() : hideValue( false ) {}
      
        QString file;
        QString group;
        QString name;
        QString label;
        QString value;
        bool hideValue;
    };
    
    Change::List changes();

    Rule::List rules();

  protected:
    /**
      Implement this function in a subclass if you want to add changes which
      can't be expressed as propagations in the kcfg file.
    */
    virtual void addCustomChanges( Change::List & ) {};

    KConfigSkeletonItem *findItem( const QString &group, const QString &name );

    QString itemValueAsString( KConfigSkeletonItem * );

    void readKcfgFile();

    Rule parsePropagation( const QDomElement &e );
    Condition parseCondition( const QDomElement &e );

    void parseConfigEntryPath( const QString &path, QString &file,
                               QString &group, QString &entry );

  private:
    KConfigSkeleton *mSkeleton;
    QString mKcfgFile;

    Rule::List mRules;    
};

#endif
