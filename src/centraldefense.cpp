#include "centraldefense.h"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QHostAddress>
#include <QRegExp>
#include <QElapsedTimer>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcCentralDefense, "jamulus.centraldefense")

CentralDefense::CentralDefense(const QUrl& blockListUrl,
                               const QUrl& asnLookupBase,
                               int refreshIntervalSeconds,
                               QObject* parent)
    : QObject(parent),
      m_blockListUrl(blockListUrl),
      m_asnLookupBase(asnLookupBase),
      m_refreshIntervalSeconds(refreshIntervalSeconds)
{
    m_nam = new QNetworkAccessManager(this);
    m_timer = new QTimer(this);
    m_timer->setInterval(qMax(1, m_refreshIntervalSeconds) * 1000);
    connect(m_timer, &QTimer::timeout, this, &CentralDefense::onTimerTick);
}

CentralDefense::~CentralDefense()
{
    stop();
    if (m_inflightBlockList) { m_inflightBlockList->abort(); m_inflightBlockList->deleteLater(); m_inflightBlockList = nullptr; }
    if (m_inflightReply) { m_inflightReply->abort(); m_inflightReply->deleteLater(); m_inflightReply = nullptr; }
    if (m_inflightTimeoutTimer) { m_inflightTimeoutTimer->stop(); m_inflightTimeoutTimer->deleteLater(); m_inflightTimeoutTimer = nullptr; }
}

void CentralDefense::start()
{
    qCInfo(lcCentralDefense) << "starting: blocklist" << m_blockListUrl.toString()
                             << "lookup_base" << m_asnLookupBase.toString()
                             << "refresh_s" << m_refreshIntervalSeconds
                             << "queue_cap" << m_maxPending;
    fetchBlockList();
    m_timer->start();
}

void CentralDefense::stop()
{
    qCInfo(lcCentralDefense) << "stopping";
    m_timer->stop();
}

void CentralDefense::onTimerTick()
{
    purgeExpired();
    fetchBlockList();
}

void CentralDefense::fetchBlockList()
{
    if (!m_blockListUrl.isValid()) {
        qCWarning(lcCentralDefense) << "invalid block list URL";
        return;
    }
    if (m_inflightBlockList) return; 

    // qCInfo(lcCentralDefense) << "blocklist: fetch start -" << m_blockListUrl.toString();
    QNetworkRequest req(m_blockListUrl);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Jamulus-CentralDefense/1.0"));
    m_inflightBlockList = m_nam->get(req);
    connect(m_inflightBlockList, &QNetworkReply::finished, this, &CentralDefense::onBlockListFetched);
}

void CentralDefense::onBlockListFetched()
{
    QNetworkReply* reply = m_inflightBlockList;
    m_inflightBlockList = nullptr;
    if (!reply) return;

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(lcCentralDefense) << "blocklist: fetch error -" << reply->errorString();
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    parseBlockList(data);
    m_lastSuccessfulFetch = QDateTime::currentDateTimeUtc();
}

void CentralDefense::parseBlockList(const QByteArray& data)
{
    QVector<Ipv4Cidr> newCidrs;
    QSet<QString> newAsns;

    QString text = QString::fromUtf8(data);
    const QStringList lines = text.split(QRegExp("[\r\n]"), Qt::SkipEmptyParts);
    int lineNo = 0;
    for (QString rawLine : lines) {
        ++lineNo;
        QString line = rawLine.trimmed();
        if (line.isEmpty()) continue;
        if (line.startsWith('#')) continue;

        // FIX: Split by whitespace and take the first token only.
        // This strips comments or names like " Datacamp Limited" from "AS212238 Datacamp Limited"
        const QString token = line.split(QRegExp("\\s+"), Qt::SkipEmptyParts).first();

        Ipv4Cidr cidr;
        // Use 'token' instead of 'line' for parsing
        if (tryParseIpv4CidrLine(token, cidr)) {
            cidr.temporary = false;
            newCidrs.append(cidr);
            continue;
        }

        // Use 'token' instead of 'line' for ASN normalization
        QString asn = normalizeAsnString(token);
        if (!asn.isEmpty()) newAsns.insert(asn);
        else qCDebug(lcCentralDefense) << "blocklist: parse ignored line" << lineNo << ":" << line;
    }

    {
        QWriteLocker locker(&m_lock);
        m_blockedCidrs = std::move(newCidrs);
        m_blockedAsns = std::move(newAsns);
    }

    // qCInfo(lcCentralDefense) << "blocklist: fetch ok - ASNs:" << m_blockedAsns.size() << "CIDRs:" << m_blockedCidrs.size();
    emit updated(m_blockedAsns.size(), m_blockedCidrs.size());
}

bool CentralDefense::tryParseIpv4CidrLine(const QString& line, Ipv4Cidr& outCidr)
{
    if (!line.contains('/')) return false;
    QStringList parts = line.split('/', Qt::SkipEmptyParts);
    if (parts.size() != 2) return false;
    QString ipPart = parts.at(0).trimmed();
    QString prefixPart = parts.at(1).trimmed();

    quint32 ip = ipv4FromString(ipPart);
    if (ip == 0 && ipPart != "0.0.0.0") return false; 

    bool ok = false;
    int prefix = prefixPart.toInt(&ok);
    if (!ok || prefix < 0 || prefix > 32) return false;

    quint32 mask = cidrMaskBits(prefix);
    quint32 network = (ip & mask);

    outCidr.network = network;
    outCidr.prefixLen = prefix;
    outCidr.temporary = false;
    return true;
}

QString CentralDefense::normalizeAsnString(const QString& s) const
{
    QString t = s.trimmed().toUpper();
    if (t.isEmpty()) return QString();
    if (t.startsWith("AS")) {
        bool ok = false; t.mid(2).toUInt(&ok);
        if (!ok) return QString();
        return t;
    } else {
        bool ok = false; t.toUInt(&ok);
        if (!ok) return QString();
        return QStringLiteral("AS%1").arg(t);
    }
}

quint32 CentralDefense::ipv4FromString(const QString& s) const
{
    QHostAddress addr(s);
    if (addr.protocol() != QAbstractSocket::IPv4Protocol) return 0;
    return addr.toIPv4Address();
}

quint32 CentralDefense::cidrMaskBits(int prefixLen) const
{
    if (prefixLen <= 0) return 0u;
    if (prefixLen >= 32) return 0xFFFFFFFFu;
    return (0xFFFFFFFFu << (32 - prefixLen));
}

bool CentralDefense::ipv4InCidr(quint32 ip, const Ipv4Cidr& cidr) const
{
    quint32 mask = cidrMaskBits(cidr.prefixLen);
    return (ip & mask) == cidr.network;
}

bool CentralDefense::isBlockedCached(const QHostAddress& addr) const
{
    QString ipStr = addr.toString();

    QReadLocker locker(&m_lock);

    auto itCache = m_ipAsnCache.constFind(ipStr);
    if (itCache != m_ipAsnCache.constEnd()) {
        if (itCache->expiry > QDateTime::currentDateTimeUtc()) {
            if (m_blockedAsns.contains(itCache->asn)) return true;
        }
    }

    if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
        quint32 ip = addr.toIPv4Address();
        for (const Ipv4Cidr& c : m_blockedCidrs) {
            if (ipv4InCidr(ip, c)) return true;
        }
    }

    return false;
}

void CentralDefense::checkAndLookup(const QHostAddress& addr)
{
    if (isBlockedCached(addr)) {
        // FIX: Verify we shout "BLOCKED" so the server can kick the user
        emit addressBlocked(addr, QStringLiteral("cached/cidr block"));
        
        emit addressChecked(addr, true, QStringLiteral("cached"));
        return;
    }

    if (addr.protocol() != QAbstractSocket::IPv4Protocol) {
        emit addressChecked(addr, false, QStringLiteral("ipv6-unsupported"));
        return;
    }

    QString ipStr = addr.toString();

    {
        QMutexLocker l(&m_pendingMutex);

        if (m_inflightIp == ipStr) {
            qCInfo(lcCentralDefense) << "lookup: coalesced (inflight)" << ipStr;
            return;
        }
        if (m_pendingSet.contains(ipStr)) {
            qCInfo(lcCentralDefense) << "lookup: coalesced (queued)" << ipStr;
            return;
        }

        if (m_pendingQueue.size() >= m_maxPending) {
            qCWarning(lcCentralDefense) << "lookup: dropped (queue full)" << ipStr << "cap=" << m_maxPending;
            emit addressChecked(addr, false, QStringLiteral("queue-full"));
            return;
        }

        m_pendingQueue.enqueue(ipStr);
        m_pendingSet.insert(ipStr);
        qCInfo(lcCentralDefense) << "lookup: enqueue" << ipStr << "queue_len=" << m_pendingQueue.size();
    }

    if (!m_inflightReply) startNextLookup();
}

void CentralDefense::startNextLookup()
{
    if (m_inflightReply) return;

    QString nextIp;
    {
        QMutexLocker l(&m_pendingMutex);
        if (m_pendingQueue.isEmpty()) return;
        nextIp = m_pendingQueue.dequeue();
        m_pendingSet.remove(nextIp);
    }

    if (nextIp.isEmpty()) return;

    QUrl url = m_asnLookupBase;
    QString path = url.path();
    if (!path.endsWith('/')) path += '/';
    path += nextIp;
    url.setPath(path);

    qCInfo(lcCentralDefense) << "lookup: start" << nextIp << "url=" << url.toString() << "pending=" << m_pendingQueue.size();
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Jamulus-CentralDefense/1.0"));

    QNetworkReply* reply = m_nam->get(req);
    m_inflightReply = reply;
    m_inflightIp = nextIp;

    connect(reply, &QNetworkReply::finished, this, &CentralDefense::onAsnLookupFinished);

    if (m_inflightTimeoutTimer) { m_inflightTimeoutTimer->stop(); m_inflightTimeoutTimer->deleteLater(); m_inflightTimeoutTimer = nullptr; }
    m_inflightTimeoutTimer = new QTimer(this);
    m_inflightTimeoutTimer->setSingleShot(true);
    connect(m_inflightTimeoutTimer, &QTimer::timeout, this, [reply]() {
        if (reply && reply->isRunning()) reply->abort();
    });
    m_inflightTimeoutTimer->start(m_lookupTimeoutSeconds * 1000);
}

void CentralDefense::onAsnLookupFinished()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) return;

    if (m_inflightTimeoutTimer) {
        m_inflightTimeoutTimer->stop();
        m_inflightTimeoutTimer->deleteLater();
        m_inflightTimeoutTimer = nullptr;
    }

    QString ip = m_inflightIp;
    m_inflightReply = nullptr;
    m_inflightIp.clear();

    if (reply->error() != QNetworkReply::NoError) {
        qCWarning(lcCentralDefense) << "lookup: error" << ip << reply->errorString();
        QHostAddress addr(ip);
        emit addressChecked(addr, false, QStringLiteral("lookup-failed"));
        reply->deleteLater();
        QTimer::singleShot(m_lookupStartSpacingMs, this, &CentralDefense::startNextLookup);
        return;
    }

    QByteArray body = reply->readAll();
    reply->deleteLater();

    QHostAddress addr(ip);
    QJsonParseError perr;
    QJsonDocument doc = QJsonDocument::fromJson(body, &perr);
    if (perr.error != QJsonParseError::NoError || !doc.isObject()) {
        qCWarning(lcCentralDefense) << "lookup: invalid JSON" << ip;
        emit addressChecked(addr, false, QStringLiteral("invalid-lookup-json"));
        QTimer::singleShot(m_lookupStartSpacingMs, this, &CentralDefense::startNextLookup);
        return;
    }

    QJsonObject obj = doc.object();
    QString asField = obj.value(QStringLiteral("as")).toString(); 
    QString asnToken;
    if (!asField.isEmpty()) {
        asnToken = asField.split(' ', Qt::SkipEmptyParts).first().toUpper();
    }

    QString normalizedAsn;
    if (!asnToken.isEmpty() && asnToken.startsWith("AS")) normalizedAsn = normalizeAsnString(asnToken);

    QDateTime now = QDateTime::currentDateTimeUtc();
    QDateTime expiry = now.addSecs(m_temporaryBlockSeconds);

    if (!normalizedAsn.isEmpty()) {
        QWriteLocker locker(&m_lock);
        AsnCacheEntry e{ normalizedAsn, expiry };
        m_ipAsnCache.insert(ip, e);
    }

    bool isBlocked = false;
    QString reason;

    if (!normalizedAsn.isEmpty()) {
        QReadLocker locker(&m_lock);
        if (m_blockedAsns.contains(normalizedAsn)) {
            isBlocked = true;
            reason = QStringLiteral("AS Block: %1").arg(normalizedAsn);
        }
    }

    if (isBlocked && addr.protocol() == QAbstractSocket::IPv4Protocol) {
        quint32 ipv4 = addr.toIPv4Address();
        quint32 mask = cidrMaskBits(24);
        quint32 net = ipv4 & mask;
        Ipv4Cidr cidr;
        cidr.network = net;
        cidr.prefixLen = 24;
        cidr.temporary = true;

        QString key = QString("%1/%2").arg(QHostAddress(cidr.network).toString()).arg(cidr.prefixLen);

        {
            QWriteLocker locker(&m_lock);
            m_blockedCidrs.append(cidr);
            m_tempCidrExpiry.insert(key, expiry);
        }

        qCInfo(lcCentralDefense) << "temp-block: add" << QHostAddress(cidr.network).toString() << "/" << cidr.prefixLen
                                 << "trigger_ip=" << addr.toString() << "asn=" << normalizedAsn
                                 << "expiry=" << expiry.toString(Qt::ISODate);

        emit addressBlocked(addr, reason);
        emit addressChecked(addr, true, reason);
    } else {
        qCInfo(lcCentralDefense) << "lookup: finish" << ip << "asn=" << normalizedAsn << "blocked=" << isBlocked;
        emit addressChecked(addr, false, QStringLiteral("asn-lookup-ok"));
    }

    QTimer::singleShot(m_lookupStartSpacingMs, this, &CentralDefense::startNextLookup);
}

void CentralDefense::purgeExpired()
{
    QDateTime now = QDateTime::currentDateTimeUtc();

    QWriteLocker locker(&m_lock);

    QList<QString> ipKeys = m_ipAsnCache.keys();
    int purgedIpCache = 0;
    for (const QString& k : ipKeys) {
        const AsnCacheEntry& e = m_ipAsnCache.value(k);
        if (e.expiry <= now) { m_ipAsnCache.remove(k); ++purgedIpCache; }
    }

    QVector<Ipv4Cidr> kept;
    int purgedCidrs = 0;
    for (const Ipv4Cidr& c : m_blockedCidrs) {
        if (!c.temporary) { kept.append(c); continue; }
        QString key = QString("%1/%2").arg(QHostAddress(c.network).toString()).arg(c.prefixLen);
        if (m_tempCidrExpiry.contains(key) && m_tempCidrExpiry.value(key) > now) {
            kept.append(c);
        } else {
            m_tempCidrExpiry.remove(key);
            ++purgedCidrs;
        }
    }
    m_blockedCidrs = std::move(kept);

    // qCDebug(lcCentralDefense) << "purge: expired ip_cache=" << purgedIpCache << "temp_cidrs=" << purgedCidrs
    //                          << "remaining_asns=" << m_blockedAsns.size() << "remaining_cidrs=" << m_blockedCidrs.size();
}

QString Ipv4Cidr::toString() const
{
    QHostAddress a(network);
    return QString("%1/%2").arg(a.toString()).arg(prefixLen);
}
