/*
    Copyright (c) 2013 Mark Gaiser <markg85@gmail.com>

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

#include <QDebug>
#include <QListView>
#include <QTreeView>
#include <kcalcore/memorycalendar.h>
#include <kcalcore/calendar.h>
#include <kcalcore/event.h>
#include <akonadi/calendar/etmcalendar.h>
#include "calendar.h"
#include <kcalendarsystem.h>

Calendar::Calendar(QObject *parent)
    : QObject(parent)
    , m_types(Holiday | Event | Todo | Journal)
    , m_sorting(None)
    , m_dayList()
    , m_weekList()
    , m_days(0)
    , m_weeks(0)
    , m_firstDayOfWeek(QLocale::system().firstDayOfWeek())
    , m_errorMessage()
{
    m_calDataPerDay = new CalendarData(this);
    m_upcommingEvents = new CalendarData(this);

    // In a calendar week view we likely want to see which events overlap. Overlapping is events that take more then 24 hours thus show in at least two days.
    // In the calendar view we don't care about that since we only want to see that events that start on the day we click. So we disable overlapping by default.
    m_calDataPerDay->setShowOverlapping(false);
    m_upcommingEvents->setShowOverlapping(true);

    m_model = new DaysModel(this);
    m_model->setSourceData(&m_dayList);

    m_dayHelper = new CalendarDayHelper(this);
    connect(m_dayHelper, SIGNAL(calendarChanged()), this, SLOT(updateData()));
}

QDate Calendar::startDate() const
{
    return m_startDate;
}

void Calendar::setStartDate(const QDate &dateTime)
{
    if(m_startDate == dateTime) {
        return;
    }
    m_startDate = dateTime;
    updateData();
    emit startDateChanged();
}

int Calendar::types() const
{
    return m_types;
}

void Calendar::setTypes(int types)
{
    if (m_types == types)
        return;

//    m_types = static_cast<Types>(types);
//    updateTypes();

    emit typesChanged();
}

int Calendar::sorting() const
{
    return m_sorting;
}

void Calendar::setSorting(int sorting)
{
    if (m_sorting == sorting)
        return;

    m_sorting = static_cast<Sorting>(sorting);

    m_calDataPerDay->setSorting(m_sorting);
    m_upcommingEvents->setSorting(m_sorting);

    emit sortingChanged();
}

int Calendar::days()
{
    return m_days;
}

void Calendar::setDays(int days)
{
    if(m_days != days) {
        m_days = days;
        updateData();
        emit daysChanged();
    }
}

int Calendar::weeks()
{
    return m_weeks;
}

void Calendar::setWeeks(int weeks)
{
    if(m_weeks != weeks) {
        m_weeks = weeks;
        updateData();
        emit weeksChanged();
    }
}

int Calendar::firstDayOfWeek()
{
    // QML has Sunday as 0, so we need to accomodate here
    return m_firstDayOfWeek == 7 ? 0 : m_firstDayOfWeek;
}

void Calendar::setFirstDayOfWeek(int day)
{
    if (day > 7) {
        return;
    }

    if (m_firstDayOfWeek != day) {
        // QML has Sunday as 0, so we need to accomodate here
        // for QDate functions which have Sunday as 7
        if (day == 0) {
            m_firstDayOfWeek = 7;
        } else {
            m_firstDayOfWeek = day;
        }

        updateData();
        emit firstDayOfWeekChanged();
    }
}

QString Calendar::errorMessage() const
{
    return m_errorMessage;
}

QString Calendar::monthName() const
{
    return QDate::longMonthName(m_startDate.month(), QDate::StandaloneFormat);
}

int Calendar::year() const
{
    return m_startDate.year();
}

QAbstractListModel *Calendar::model() const
{
    return m_model;
}

QAbstractItemModel *Calendar::selectedDayModel() const
{
    return m_calDataPerDay->model();
}

QAbstractItemModel *Calendar::upcomingEventsModel() const
{
    return m_upcommingEvents->model();
}

QList<int> Calendar::weeksModel() const
{
    return m_weekList;
}

void Calendar::updateData()
{
    /**
     * Algorithm description
     * 
     * Impoertant note, this function should only be called as a result of a month change or bigger (year change).
     * 
     * Step 1: We calculate the number of days we have
     * Step 2: Calculate the number of days we can show before the current month. Here we take the following argument into account.
     *         In month views you - at most - see one full week of the previous month. That is when the next month starts on the 
     *         same day as our week days start. So if a month' 1st day is on monday then we have one full week to show from the
     *         last month. Otherwise we show the number of days till the starting day of the week. So if our first day of a month
     *         is Wednesday (which is 3) then we have 2 days to show from the last month.
     * Step 3: We now now how many days we need to show from the last month. Do that by taking the last day from the last month
     *         and just count down the number of times you have free spots. In our ebove example with wednesday that would be 2,
     * Step 4: Then we fill out entire current month. Not much difficulties there.
     * Step 5: After this we might have some free spots left (remember, by default we fill 42 spots). Those that are left are
     *         filled by the days from the next month.
     * Step 6: Afterwrds we tell the model that our data has been changed and update the upcomingEventsModel since that has now 
     *         also been changed.
     */
    
    if(m_days == 0 || m_weeks == 0) {
        return;
    }

    m_dayList.clear();
    m_weekList.clear();

    m_dayHelper->setDate(m_startDate.year(), m_startDate.month());

    int totalDays = m_days * m_weeks;

    int daysBeforeCurrentMonth = 0;
    int daysAfterCurrentMonth = 0;

    QDate firstDay(m_startDate.year(), m_startDate.month(), 1);


    // If the first day is the same as the starting day then we add a complete row before it.
    daysBeforeCurrentMonth = firstDay.dayOfWeek() - m_firstDayOfWeek;


    int daysThusFar = daysBeforeCurrentMonth + m_startDate.daysInMonth();
    if(daysThusFar < totalDays) {
        daysAfterCurrentMonth = totalDays - daysThusFar;
    }

    if(daysBeforeCurrentMonth > 0) {
        QDate previousMonth = m_startDate.addMonths(-1);
        for(int i = 0; i < daysBeforeCurrentMonth; i++) {
            DayData day;
            day.isCurrentMonth = false;
            day.isNextMonth = false;
            day.isPreviousMonth = true;
            day.dayNumber = previousMonth.daysInMonth() - (daysBeforeCurrentMonth - (i + 1));
            day.monthNumber = previousMonth.month();
            day.yearNumber = previousMonth.year();
            day.containsEventItems = false;
            m_dayList << day;
        }
    }

    for(int i = 0; i < m_startDate.daysInMonth(); i++) {
        DayData day;
        day.isCurrentMonth = true;
        day.isNextMonth = false;
        day.isPreviousMonth = false;
        day.dayNumber = i + 1; // +1 to go form 0 based index to 1 based calendar dates
        day.monthNumber = m_startDate.month();
        day.yearNumber = m_startDate.year();
        day.containsEventItems = m_dayHelper->containsEventItems(i + 1);
        m_dayList << day;
    }

    if(daysAfterCurrentMonth > 0) {
        for(int i = 0; i < daysAfterCurrentMonth; i++) {
            DayData day;
            day.isCurrentMonth = false;
            day.isNextMonth = true;
            day.isPreviousMonth = false;
            day.dayNumber = i + 1; // +1 to go form 0 based index to 1 based calendar dates
            day.monthNumber = m_startDate.addMonths(1).month();
            day.yearNumber = m_startDate.addMonths(1).year();
            day.containsEventItems = false;
            m_dayList << day;
        }
    }

    const int numOfDaysInCalendar = m_dayList.count();

    // Fill weeksModel (just a QList<int>) with the week numbers. This needs some tweaking!
    for(int i = 0; i < numOfDaysInCalendar; i += 7) {
        const DayData& data = m_dayList.at(i);
        m_weekList << QDate(data.yearNumber, data.monthNumber, data.dayNumber).weekNumber();
    }

    m_model->update();

    // Update the upcomming events model according to the new start date.
    upcommingEventsFromDay(m_startDate.year(), m_startDate.month(), m_startDate.day());

//    qDebug() << "---------------------------------------------------------------";
//    qDebug() << "Date obj: " << m_startDate;
//    qDebug() << "Month: " << m_startDate.month();
//    qDebug() << "m_days: " << m_days;
//    qDebug() << "m_weeks: " << m_weeks;
//    qDebug() << "Days before this month: " << daysBeforeCurrentMonth;
//    qDebug() << "Days after this month: " << daysAfterCurrentMonth;
//    qDebug() << "Days in current month: " << m_startDate.daysInMonth();
//    qDebug() << "m_dayList size: " << m_dayList.count();
//    qDebug() << "---------------------------------------------------------------";
}

void Calendar::nextMonth()
{
    m_startDate = m_startDate.addMonths(1);
    updateData();
    emit startDateChanged();
}

QString Calendar::dayName(int weekday) const
{
    return QDate::shortDayName(weekday);
}

void Calendar::nextYear()
{
    m_startDate = m_startDate.addYears(1);
    updateData();
    emit startDateChanged();
}

void Calendar::previousYear()
{
    m_startDate = m_startDate.addYears(-1);
    updateData();
    emit startDateChanged();
}

void Calendar::previousMonth()
{
    m_startDate = m_startDate.addMonths(-1);
    updateData();
    emit startDateChanged();
}

void Calendar::setSelectedDay(int year, int month, int day) const
{
    m_calDataPerDay->setStartDate(QDate(year, month, day));
    m_calDataPerDay->setEndDate(m_calDataPerDay->startDate().addDays(1));
}

void Calendar::upcommingEventsFromDay(int year, int month, int day) const
{
    m_upcommingEvents->setStartDate(QDate(year, month, day));
    m_upcommingEvents->setEndDate(m_upcommingEvents->startDate().addMonths(1));
}
