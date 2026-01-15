#pragma once

#include <QObject>
#include <QUrl>
#include <QHostAddress>
#include <QVector>
#include <QSet>
#include <QHash>
#include <QDateTime>
#include <QTimer>
#include <QReadWriteLock>
#include <QNetworkAccessManager>
#include <QQueue>
#include <QMutex>

struct Ipv4Cidr
{
    quint32 network;
    int prefixLen;
    bool temporary;
    QString toString() const;
};

Q_DECLARE_TYPEINFO(Ipv4Cidr, Q_MOVABLE_TYPE);

class CentralDefense : public QObject
{
    Q_OBJECT

public:
    explicit CentralDefense(const QUrl& blockListUrl,
                            const QUrl& asnLookupBase = QUrl("http://ip-api.com/json/"),
                            int refreshIntervalSeconds = 60,
                            QObject* parent = nullptr);
    ~CentralDefense() override;

    void start();
    void stop();

    bool isBlockedCached(const QHostAddress& addr) const;
    void checkAndLookup(const QHostAddress& addr);
    void setTemporaryBlockSeconds(int seconds) { m_temporaryBlockSeconds = seconds; }

signals:
    void addressChecked(const QHostAddress& addr, bool isBlocked, const QString& reason);
    void addressBlocked(const QHostAddress& addr, const QString& reason);
    void updated(int numAsns, int numCidrs);

private slots:
    void onBlockListFetched();
    void onAsnLookupFinished();
    void onTimerTick();

private:
    void fetchBlockList();
    void parseBlockList(const QByteArray& data);
    bool tryParseIpv4CidrLine(const QString& line, Ipv4Cidr& outCidr);
    QString normalizeAsnString(const QString& s) const;
    quint32 ipv4FromString(const QString& s) const;
    quint32 cidrMaskBits(int prefixLen) const;
    bool ipv4InCidr(quint32 ip, const Ipv4Cidr& cidr) const;
    void purgeExpired();
    void startNextLookup();

private:
    QUrl m_blockListUrl;
    QUrl m_asnLookupBase;
    int m_refreshIntervalSeconds;

    QNetworkAccessManager* m_nam;
    QTimer* m_timer;

    mutable QReadWriteLock m_lock;
    QSet<QString> m_blockedAsns;
    QVector<Ipv4Cidr> m_blockedCidrs;

    struct AsnCacheEntry { QString asn; QDateTime expiry; };
    QHash<QString, AsnCacheEntry> m_ipAsnCache;
    QHash<QString, QDateTime> m_tempCidrExpiry;

    QQueue<QString> m_pendingQueue;
    QSet<QString> m_pendingSet;
    QMutex m_pendingMutex;
    int m_maxPending = 25;

    QNetworkReply* m_inflightReply = nullptr;
    QString m_inflightIp;
    QTimer* m_inflightTimeoutTimer = nullptr;
    int m_lookupStartSpacingMs = 1000;
    int m_lookupTimeoutSeconds = 10;
    int m_temporaryBlockSeconds = 24 * 3600; 

    QNetworkReply* m_inflightBlockList = nullptr;
    QDateTime m_lastSuccessfulFetch;
};
