#include "wizDocumentListViewItem.h"

#include <QFile>
#include <QFileInfo>

#include <QPixmapCache>
#include <QPainter>
#include <QStyleOptionViewItemV4>

#include "wizDocumentListView.h"
#include "share/wizDatabaseManager.h"
#include "share/wizDatabase.h"
#include "share/wizsettings.h"
#include "wizPopupButton.h"

#include "thumbcache.h"
#include "sync/avatar.h"
#include "utils/stylehelper.h"

using namespace Core;

CWizDocumentListViewItem::CWizDocumentListViewItem(CWizExplorerApp& app,
                                                   const WizDocumentListViewItemData& data)
    : QListWidgetItem(0, UserType)
    , QObject(0)
    , m_app(app)
    , m_nSortingType(0)
    , m_nSize(0)
    , m_documentUnread(false)
    , m_specialFocused(false)
{
    Q_ASSERT(!data.doc.strKbGUID.isEmpty());
    Q_ASSERT(!data.doc.strGUID.isEmpty());

    m_data.nType = data.nType;
    m_data.doc = data.doc;
    m_data.nMessageId = data.nMessageId;
    m_data.nReadStatus = data.nReadStatus;
    m_data.strAuthorId = data.strAuthorId;

    setText(data.doc.strTitle);

    updateDocumentUnreadCount();

    connect(this, SIGNAL(thumbnailReloaded()), SLOT(on_thumbnailReloaded()));
}

void CWizDocumentListViewItem::resetAvatar(const QString& strFileName)
{
    Q_EMIT thumbnailReloaded();
}

bool CWizDocumentListViewItem::isAvatarNeedUpdate(const QString& strFileName)
{
    if (!QFile::exists(strFileName)) {
        return true;
    }

    QFileInfo info(strFileName);

    QDateTime tCreated = info.created();
    QDateTime tNow = QDateTime::currentDateTime();
    if (tCreated.daysTo(tNow) >= 1) { // download avatar before yesterday
        return true;
    }

    return false;
}

bool CWizDocumentListViewItem::isContainsAttachment() const
{
    WIZDOCUMENTDATA docData;
    if (m_app.databaseManager().db(m_data.doc.strKbGUID).DocumentFromGUID(m_data.doc.strGUID, docData))
    {
        return docData.nAttachmentCount > 0;
    }
    return false;
}
bool CWizDocumentListViewItem::isSpecialFocus() const
{
    return m_specialFocused;
}

void CWizDocumentListViewItem::setSpecialFocused(bool bSpecialFocus)
{
    m_specialFocused = bSpecialFocus;
}

void CWizDocumentListViewItem::updateDocumentUnreadCount()
{
    if (m_data.doc.strKbGUID != CWizDatabaseManager::instance()->db().kbGUID())
    {
        m_documentUnread = (m_data.doc.nReadCount == 0);
    }
}

void CWizDocumentListViewItem::resetAbstract(const WIZABSTRACT& abs)
{
    m_data.thumb.strKbGUID = abs.strKbGUID;
    m_data.thumb.guid= abs.guid;
    m_data.thumb.text = abs.text;
    m_data.thumb.image = abs.image;

    Q_EMIT thumbnailReloaded();
}

const QString& CWizDocumentListViewItem::tags()
{
    if (m_strTags.isEmpty()) {
        m_strTags = m_app.databaseManager().db(m_data.doc.strKbGUID).GetDocumentTagDisplayNameText(m_data.doc.strGUID);
    }

    return m_strTags;
}

const QString& CWizDocumentListViewItem::tagTree()
{
    if (m_strTags.isEmpty()) {
        m_strTags = "/" + m_app.databaseManager().db(m_data.doc.strKbGUID).name();
        m_strTags += m_app.databaseManager().db(m_data.doc.strKbGUID).GetDocumentTagTreeDisplayString(m_data.doc.strGUID);
    }

    return m_strTags;
}

void CWizDocumentListViewItem::reload(CWizDatabase& db)
{
    Q_ASSERT(db.kbGUID() == m_data.doc.strKbGUID);

    m_strTags.clear();
    m_data.thumb = WIZABSTRACT();
    setSortingType(m_nSortingType); // reset info

    db.DocumentFromGUID(m_data.doc.strGUID, m_data.doc);
    setText(m_data.doc.strTitle);
    updateDocumentUnreadCount();

    Q_EMIT thumbnailReloaded();
}

void CWizDocumentListViewItem::setSortingType(int type)
{
    m_nSortingType = type;

    QString strFileName = m_app.databaseManager().db(m_data.doc.strKbGUID).GetDocumentFileName(m_data.doc.strGUID);
    QFileInfo fi(strFileName);

    if (m_data.nType == TypeGroupDocument) {
        switch (m_nSortingType) {
        case CWizSortingPopupButton::SortingCreateTime:
        case -CWizSortingPopupButton::SortingCreateTime:
            m_data.strInfo = m_data.doc.tCreated.toHumanFriendlyString();
            break;
        case CWizSortingPopupButton::SortingUpdateTime:
        case -CWizSortingPopupButton::SortingUpdateTime:
            m_data.strInfo = m_data.doc.tModified.toHumanFriendlyString();
            break;
        case CWizSortingPopupButton::SortingTitle:
        case -CWizSortingPopupButton::SortingTitle:
            m_data.strInfo = m_data.doc.tModified.toHumanFriendlyString();
            break;
        case CWizSortingPopupButton::SortingTag:
        case -CWizSortingPopupButton::SortingTag:
            m_data.strInfo = tagTree();
            break;
        case CWizSortingPopupButton::SortingLocation:
        case -CWizSortingPopupButton::SortingLocation:
            m_data.strInfo = tagTree();
            break;
        case CWizSortingPopupButton::SortingSize:
        case -CWizSortingPopupButton::SortingSize:
            if (!fi.exists()) {
                m_data.strInfo = QObject::tr("Unknown");
            } else {
                m_nSize = fi.size();
                m_data.strInfo = ::WizGetFileSizeHumanReadalbe(strFileName);
            }
            break;
        default:
            Q_ASSERT(0);
            break;
        }
    } else if (m_data.nType == TypePrivateDocument) {
        switch (m_nSortingType) {
        case CWizSortingPopupButton::SortingCreateTime:
        case -CWizSortingPopupButton::SortingCreateTime:
            m_data.strInfo = m_data.doc.tCreated.toHumanFriendlyString() + " " + tags();
            break;
        case CWizSortingPopupButton::SortingUpdateTime:
        case -CWizSortingPopupButton::SortingUpdateTime:
            m_data.strInfo = m_data.doc.tModified.toHumanFriendlyString() + " " + tags();
            break;
        case CWizSortingPopupButton::SortingTitle:
        case -CWizSortingPopupButton::SortingTitle:
            m_data.strInfo = m_data.doc.tModified.toHumanFriendlyString() + " " + tags();
            break;
        case CWizSortingPopupButton::SortingTag:
        case -CWizSortingPopupButton::SortingTag:
            m_data.strInfo = m_data.doc.tModified.toHumanFriendlyString() + " " + tags();
            break;
        case CWizSortingPopupButton::SortingLocation:
        case -CWizSortingPopupButton::SortingLocation:
            m_data.strInfo = ::WizLocation2Display(m_data.doc.strLocation);
            break;
        case CWizSortingPopupButton::SortingSize:
        case -CWizSortingPopupButton::SortingSize:
            if (!fi.exists()) {
                m_data.strInfo = QObject::tr("Unknown") + " " + tags();
            } else {
                m_nSize = fi.size();
                m_data.strInfo = ::WizGetFileSizeHumanReadalbe(strFileName) + " "  + tags();
            }
            break;
        default:
            Q_ASSERT(0);
            break;
        }
    }
}

bool CWizDocumentListViewItem::operator <(const QListWidgetItem &other) const
{
    const CWizDocumentListViewItem* pOther = dynamic_cast<const CWizDocumentListViewItem*>(&other);
    Q_ASSERT(pOther && m_nSortingType == pOther->m_nSortingType);


    switch (m_nSortingType) {
    case CWizSortingPopupButton::SortingCreateTime:
        // default compare use create time
        return pOther->m_data.doc.tCreated < m_data.doc.tCreated;
    case -CWizSortingPopupButton::SortingCreateTime:
        return pOther->m_data.doc.tCreated > m_data.doc.tCreated;
    case CWizSortingPopupButton::SortingUpdateTime:
        return pOther->m_data.doc.tModified < m_data.doc.tModified;
    case -CWizSortingPopupButton::SortingUpdateTime:
        return pOther->m_data.doc.tModified > m_data.doc.tModified;
    case CWizSortingPopupButton::SortingTitle:
        return pOther->m_data.doc.strTitle.localeAwareCompare(m_data.doc.strTitle) < 0;
    case -CWizSortingPopupButton::SortingTitle:
        return pOther->m_data.doc.strTitle.localeAwareCompare(m_data.doc.strTitle) > 0;
    case CWizSortingPopupButton::SortingLocation:
        return pOther->m_data.strInfo.localeAwareCompare(m_data.strInfo) < 0;
    case -CWizSortingPopupButton::SortingLocation:
        return pOther->m_data.strInfo.localeAwareCompare(m_data.strInfo) > 0;
    case CWizSortingPopupButton::SortingTag:
        return pOther->m_strTags < m_strTags;
    case -CWizSortingPopupButton::SortingTag:
        return pOther->m_strTags > m_strTags;
    case CWizSortingPopupButton::SortingSize:
        return pOther->m_nSize < m_nSize;
    case -CWizSortingPopupButton::SortingSize:
        return pOther->m_nSize > m_nSize;
    default:
        Q_ASSERT(0);
    }

    return true;
}

void CWizDocumentListViewItem::on_thumbnailReloaded()
{
    QPixmapCache::remove(m_data.doc.strGUID + ":normal");
    QPixmapCache::remove(m_data.doc.strGUID + ":focus");
    QPixmapCache::remove(m_data.doc.strGUID + ":nofocus");
}

//void CWizDocumentListViewItem::onThumbCacheLoaded(const QString& strKbGUID, const QString& strGUID)
//{
//    if (strKbGUID == m_data.doc.strKbGUID && strGUID == m_data.doc.strGUID)
//        setNeedUpdate();
//}

void CWizDocumentListViewItem::draw(QPainter* p, const QStyleOptionViewItemV4* vopt, int nViewType) const
{
    int nItemType = itemType();
    draw_impl(p, vopt, nItemType, nViewType);
    drawSyncStatus(p, vopt, nViewType);
}

void CWizDocumentListViewItem::draw_impl(QPainter* p, const QStyleOptionViewItemV4* vopt, int nItemType, int nViewType) const
{
    if (nItemType == CWizDocumentListViewItem::TypePrivateDocument)
    {
        switch (nViewType) {
        case CWizDocumentListView::TypeThumbnail:
            drawPrivateSummaryView_impl(p, vopt);
            return;
        case CWizDocumentListView::TypeTwoLine:
            drawPrivateTwoLineView_impl(p, vopt);
            return;
        case CWizDocumentListView::TypeOneLine:
            drawOneLineView_impl(p, vopt);
            return;
        default:
            Q_ASSERT(0);
            return;
        }
    }
    else if (nItemType == CWizDocumentListViewItem::TypeGroupDocument)
    {
        switch (nViewType) {
        case CWizDocumentListView::TypeThumbnail:
            drawGroupSummaryView_impl(p, vopt);
            return;
        case CWizDocumentListView::TypeTwoLine:
            drawGroupTwoLineView_impl(p, vopt);
            return;
        case CWizDocumentListView::TypeOneLine:
            drawOneLineView_impl(p, vopt);
            return;
        default:
            Q_ASSERT(0);
            return;
        }
    }
    Q_ASSERT(0);
}

void CWizDocumentListViewItem::setNeedUpdate() const
{
    QPixmapCache::remove(cacheKey());
}

QString CWizDocumentListViewItem::cacheKey() const
{
    CWizDocumentListView* view = qobject_cast<CWizDocumentListView*>(listWidget());
    Q_ASSERT(view);

    QString stat;
    if (isSelected()) {
        if (view->hasFocus())
            stat = "Focus";
        else
            stat = "LoseFocus";
    } else {
        stat = "Normal";
    }

    return "Core::ListItem::" + m_data.doc.strGUID + "::" + view->viewType() + "::" + stat;
}

void CWizDocumentListViewItem::drawPrivateSummaryView_impl(QPainter* p, const QStyleOptionViewItemV4* vopt) const
{
    bool bSelected = vopt->state & QStyle::State_Selected;
    bool bFocused = listWidget()->hasFocus();

    WIZABSTRACT thumb;
    ThumbCache::instance()->find(m_data.doc.strKbGUID, m_data.doc.strGUID, thumb);

    QRect rcd = drawItemBackground(p, vopt->rect, bSelected, bFocused);

    if (!thumb.image.isNull()) {
        QPixmap pmt = QPixmap::fromImage(thumb.image);
        QRect rcp = Utils::StyleHelper::drawThumbnailPixmap(p, rcd, pmt);
        rcd.setRight(rcp.left());
    }

    int nType = m_data.doc.nProtected ? Utils::StyleHelper::BadgeEncryted : Utils::StyleHelper::BadgeNormal;
    bool bContainsAttach = isContainsAttachment();
    Utils::StyleHelper::drawListViewItemThumb(p, rcd, nType, m_data.doc.strTitle, m_data.strInfo, thumb.text, bFocused, bSelected, bContainsAttach);
}

void CWizDocumentListViewItem::drawGroupSummaryView_impl(QPainter* p, const QStyleOptionViewItemV4* vopt) const
{
    bool bSelected = vopt->state & QStyle::State_Selected;
    bool bFocused = listWidget()->hasFocus();

    WIZABSTRACT thumb;
    ThumbCache::instance()->find(m_data.doc.strKbGUID, m_data.doc.strGUID, thumb);

    QRect rcd = drawItemBackground(p, vopt->rect, bSelected, bFocused);

    QPixmap pmAvatar;
    WizService::AvatarHost::avatar(m_data.strAuthorId, &pmAvatar);
    QRect rcAvatar = Utils::StyleHelper::drawAvatar(p, rcd, pmAvatar);
    int nAvatarRightMargin = 4;
    rcd.setLeft(rcAvatar.right() + nAvatarRightMargin);

    int nType = m_data.doc.nProtected ? Utils::StyleHelper::BadgeEncryted : Utils::StyleHelper::BadgeNormal;
    bool bContainsAttach = isContainsAttachment();
    Utils::StyleHelper::drawListViewItemThumb(p, rcd, nType, m_data.doc.strTitle, m_data.strInfo, thumb.text, bFocused, bSelected, bContainsAttach);
}

void CWizDocumentListViewItem::drawPrivateTwoLineView_impl(QPainter* p, const QStyleOptionViewItemV4* vopt) const
{
    bool bSelected = vopt->state & QStyle::State_Selected;
    bool bFocused = listWidget()->hasFocus();

    QRect rcd = drawItemBackground(p, vopt->rect, bSelected, bFocused);

    int nType = m_data.doc.nProtected ? Utils::StyleHelper::BadgeEncryted : Utils::StyleHelper::BadgeNormal;
    bool bContainsAttach = isContainsAttachment();
    Utils::StyleHelper::drawListViewItemThumb(p, rcd, nType, m_data.doc.strTitle, m_data.strInfo, NULL, bFocused, bSelected, bContainsAttach);
}

void CWizDocumentListViewItem::drawGroupTwoLineView_impl(QPainter* p, const QStyleOptionViewItemV4* vopt) const
{
    bool bSelected = vopt->state & QStyle::State_Selected;
    bool bFocused = listWidget()->hasFocus();

    QRect rcd = drawItemBackground(p, vopt->rect, bSelected, bFocused);

    QPixmap pmAvatar;
    WizService::AvatarHost::avatar(m_data.strAuthorId, &pmAvatar);
    QRect rcAvatar = Utils::StyleHelper::drawAvatar(p, rcd, pmAvatar);
    int nAvatarRightMargin = 4;
    rcd.setLeft(rcAvatar.right() + nAvatarRightMargin);

    int nType = m_data.doc.nProtected ? Utils::StyleHelper::BadgeEncryted : Utils::StyleHelper::BadgeNormal;
    bool bContainsAttach = isContainsAttachment();
    Utils::StyleHelper::drawListViewItemThumb(p, rcd, nType, m_data.doc.strTitle, m_data.strInfo, NULL, bFocused, bSelected, bContainsAttach);
}

void CWizDocumentListViewItem::drawOneLineView_impl(QPainter* p, const  QStyleOptionViewItemV4* vopt) const
{
    bool bSelected = vopt->state & QStyle::State_Selected;
    bool bFocused = listWidget()->hasFocus();

    QRect rcd = drawItemBackground(p, vopt->rect, bSelected, bFocused);

    int nType = m_data.doc.nProtected ? Utils::StyleHelper::BadgeEncryted : Utils::StyleHelper::BadgeNormal;
    bool bContainsAttach = isContainsAttachment();
    Utils::StyleHelper::drawListViewItemThumb(p, rcd, nType, m_data.doc.strTitle, NULL, NULL, bFocused, bSelected, bContainsAttach);
}

void CWizDocumentListViewItem::drawSyncStatus(QPainter* p, const QStyleOptionViewItemV4* vopt, int nViewType) const
{
    Q_UNUSED(nViewType);

    QString strIconPath;
    CWizDatabase& db = m_app.databaseManager().db(m_data.doc.strKbGUID);
    if (db.IsDocumentModified(m_data.doc.strGUID))
    {
        strIconPath = ::WizGetSkinResourcePath(m_app.userSettings().skin()) + "uploading.bmp";
    }
    else if (!db.IsDocumentDownloaded(m_data.doc.strGUID))
    {
        strIconPath = ::WizGetSkinResourcePath(m_app.userSettings().skin()) + "downloading.bmp";
    }
    else
        return;

    p->save();
    int nMargin = -1;
    QPixmap fullPic(strIconPath);
    QPixmap pix = fullPic.copy(0, 0, fullPic.height(), fullPic.height());
    pix.setMask(pix.createMaskFromColor(Qt::black, Qt::MaskInColor));
    QRect rcSync(vopt->rect.right() - pix.width() - nMargin, vopt->rect.bottom() - pix.height() - nMargin,
                 pix.width(), pix.height());
    p->drawPixmap(rcSync, pix);
    p->restore();

    return;
}

QRect CWizDocumentListViewItem::drawItemBackground(QPainter* p, const QRect& rect, bool selected, bool focused) const
{
    if (selected && focused)
    {
        return Utils::StyleHelper::initListViewItemPainter(p, rect,Utils::StyleHelper::ListBGTypeActive);
    }
    else if ((selected && !focused) || m_specialFocused)
    {
        return Utils::StyleHelper::initListViewItemPainter(p, rect,  Utils::StyleHelper::ListBGTypeHalfActive);
    }
    else if (m_documentUnread)
    {
        return Utils::StyleHelper::initListViewItemPainter(p, rect, Utils::StyleHelper::ListBGTypeUnread);
    }

    return Utils::StyleHelper::initListViewItemPainter(p, rect, Utils::StyleHelper::ListBGTypeNone);
}
