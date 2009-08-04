/****************************************************************************** * *
 *
 *  File : rulelisteditor.h
 *  Created on Fri 15 May 2009 04:53:16 by Szymon Tomasz Stefanek
 *
 *  This file is part of the Akonadi Filtering Framework
 *
 *  Copyright 2009 Szymon Tomasz Stefanek <pragma@kvirc.net>
 *
 *  This rulelist is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This rulelist is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the editoried warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this rulelist; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *******************************************************************************/

#ifndef _AKONADI_FILTER_UI_RULELISTEDITOR_H_
#define _AKONADI_FILTER_UI_RULELISTEDITOR_H_

#include <akonadi/filter/ui/config-akonadi-filter-ui.h>

#include <akonadi/filter/ui/actioneditor.h>

namespace Akonadi
{
namespace Filter
{
namespace Action
{
  class RuleList;
} // namespace Action

namespace UI
{

class RuleListEditorTBB;
class RuleListEditorLBB;

class AKONADI_FILTER_UI_EXPORT RuleListEditor : public ActionEditor
{
  Q_OBJECT
public:
  enum EditorStyle
  {
    ToolBoxBased,
    ListBoxBased
  };
public:
  RuleListEditor(
      QWidget * parent,
      ComponentFactory * componentfactory,
      EditorFactory * editorComponentFactory,
      EditorStyle style = ToolBoxBased
    );
  virtual ~RuleListEditor();
protected:
  RuleListEditorTBB * mToolBoxBasedEditor;
  RuleListEditorLBB * mListBoxBasedEditor;
  EditorStyle mStyle;
public:
  void setAutoExpand( bool b );
  bool autoExpand() const;
  virtual void fillFromAction( Action::Base * action );
  virtual void fillFromRuleList( Action::RuleList * ruleList );
  bool commitStateToRuleList( Action::RuleList * ruleList );
  virtual Action::Base * commitState( Component * parent );
};

} // namespace UI

} // namespace Filter

} // namespace Akonadi

#endif //!_AKONADI_FILTER_UI_RULELISTEDITOR_H_
