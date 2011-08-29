#ifndef KRSSLOCALRESOURCE_H
#define KRSSLOCALRESOURCE_H

#include <akonadi/cachepolicy.h>
#include <akonadi/resourcebase.h>
#include <boost/shared_ptr.hpp>
#include <krssresource/opmlparser.h>
#include <qtimer.h>
#include <Syndication/Syndication>

class KRssLocalResource : public Akonadi::ResourceBase,
                           public Akonadi::AgentBase::Observer
{
  Q_OBJECT

  public:
    KRssLocalResource( const QString &id );
    ~KRssLocalResource();
    Akonadi::Collection::List buildCollectionTree( QList<boost::shared_ptr<const KRssResource::ParsedNode> > listOfNodes, 
 				   Akonadi::Collection::List &list, Akonadi::Collection &parent);
    QString mimeType();

  public Q_SLOTS:
    virtual void configure( WId windowId );

  protected Q_SLOTS:
    void retrieveCollections();
    void retrieveItems( const Akonadi::Collection &col );
    bool retrieveItem( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    void slotLoadingComplete(Syndication::Loader* loader, Syndication::FeedPtr feed, 
			Syndication::ErrorCode status );
//   bool jobFinished( KJob *job );
    
  protected:
    virtual void aboutToQuit();

    virtual void itemAdded( const Akonadi::Item &item, const Akonadi::Collection &collection );
    virtual void itemChanged( const Akonadi::Item &item, const QSet<QByteArray> &parts );
    virtual void itemRemoved( const Akonadi::Item &item );
    
    void fetchFeed( const Akonadi::Collection &feed );
    
  private:    
    Akonadi::CachePolicy policy;
    QList<Syndication::ItemPtr> m_syndItems;
    static const int CACHE_TIMEOUT = -1, INTERVAL_CHECK_TIME = 5;
    
};

#endif
