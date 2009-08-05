/****************************************************************************** *
 *
 *  File : rule.h
 *  Created on Thu 07 May 2009 13:30:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filtering Framework
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef _AKONADI_FILTER_RULE_H_
#define _AKONADI_FILTER_RULE_H_

#include <akonadi/filter/config-akonadi-filter.h>

#include <QtCore/QString>
#include <QtCore/QList>

#include <akonadi/filter/action.h>
#include <akonadi/filter/component.h>
#include <akonadi/filter/propertybag.h>

namespace Akonadi
{
namespace Filter
{

class Data;

namespace Condition
{
  class Base;
} // namespace Condition

/**
 * A single rule in the filtering program.
 *
 * A rule is made of a condition and a set of actions.
 * If the condition matches then the actions are executed in sequence.
 */
class AKONADI_FILTER_EXPORT Rule : public Component, public PropertyBag
{
public:
  Rule( Component * parent );

  virtual ~Rule();

protected:

  /**
   * The condition attached to this rule. Owned pointer. May be null (null condition is always true)
   */
  Condition::Base * mCondition;

  /**
   * The list of actions attachhed to this rule, owned pointers. May be empty.
   */
  QList< Action::Base * > mActionList;

public:

  /**
   * Returns the description of this program.
   * This is equivalent to property( QString::fromAscii( "description" ) ).
   */
  QString description() const;

  /**
   * Sets the user supplied name of this filtering program.
   * This is equivalent to setProperty( QString::fromAscii( "description" ), description ).
   */
  void setDescription( const QString &description );

  virtual bool isRule() const;

  bool isEmpty() const;

  Condition::Base * condition() const
  {
    return mCondition;
  }

  QList< Action::Base * > * actionList()
  {
    return &mActionList;
  }

  void clearActionList()
  {
    mActionList.clear();
  }

  void addAction( Action::Base * action )
  {
    mActionList.append( action );
  }

  void setCondition( Condition::Base * condition );

  virtual ProcessingStatus execute( Data * data );

  virtual void dump( const QString &prefix );
};

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_RULE_H_
