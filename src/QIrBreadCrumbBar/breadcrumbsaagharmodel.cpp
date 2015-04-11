/**********************************************************************
**
** This file is part of QIron Toolkit.
**
** Copyright (C) 2009-2020 Dzimi Mve Alexandre <dzimiwine@gmail.com>
**
** Contact: dzimiwine@gmail.com
**
** QIron is a free toolkit developed in Qt by Dzimi Mve A.; you can redistribute
** sources and libraries of this library and/or modify them under the terms of
** the GNU Library General Public License version 3.0 as published by the
** Free Software Foundation and appearing in the file LICENSE.txt included in
** the packaging of this file.
** Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** This SDK is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Library General Public License for more details.
**
**********************************************************************/

#include "breadcrumbsaagharmodel.h"
#include "saagharapplication.h"
#include "databasebrowser.h"
#include "outlinemodel.h"
#include "saagharwidget.h"

#include <QAction>
#include <QMenu>
#include <QModelIndex>
#include <QIcon>
#include <QMimeData>

QIR_BEGIN_NAMESPACE

const static QString SEPARATOR = QLatin1String("/");

BreadCrumbSaagharModel::BreadCrumbSaagharModel()
{
    setItemModel(sApp->outlineModel());
}

QString BreadCrumbSaagharModel::defaultPath() const
{
    return (SEPARATOR + SaagharWidget::rootTitle() + SEPARATOR);
}

QString BreadCrumbSaagharModel::cleanPath(const QString &path) const
{
    QStringList sections = pathSections(path);

    if (!sections.isEmpty() && sections.at(0).compare(SaagharWidget::rootTitle(), Qt::CaseInsensitive) == 0) {
        sections.removeAt(0);
    }

    sections.prepend(SaagharWidget::rootTitle());

    return sections.join(SEPARATOR).prepend(SEPARATOR).append(SEPARATOR);
}

bool BreadCrumbSaagharModel::isValid(const QString &path) const
{
    return sApp->outlineModel()->isPathValid(pathSections(cleanPath(path)));;
}

QIrBreadCrumbModelNodeList BreadCrumbSaagharModel::splitPath(const QString &path) const
{
    QIrBreadCrumbModelNodeList nodeList;

    const QStringList sections = pathSections(path);
    QIrBreadCrumbModelNode::Type nodeType;

    for (int i = 0; i < sections.size(); ++i) {
        if (i == 0) {
            nodeType = QIrBreadCrumbModelNode::Root;
        }
        else if (i != sections.size() - 1) {
            nodeType = QIrBreadCrumbModelNode::Container;
        }
        else {
            nodeType = pathNodeType(sections);
        }

        nodeList.append(QIrBreadCrumbModelNode(QStringList(sections.mid(0, i + 1)).join(SEPARATOR), nodeType, this));
    }

    return nodeList;
}

QIcon BreadCrumbSaagharModel::icon(const QIrBreadCrumbModelNode &node) const
{
    return icon(node.type());
}

QString BreadCrumbSaagharModel::label(const QIrBreadCrumbModelNode &node) const
{
    const QStringList sections = pathSections(node.path());

    if (sections.isEmpty()) {
        return QString();
    }
    else {
        return sections.last();
    }
}

QMimeData* BreadCrumbSaagharModel::mimeData(const QIrBreadCrumbModelNode &) const
{
    return 0;
}

QMenu* BreadCrumbSaagharModel::buildMenu(const QIrBreadCrumbModelNode &node)
{
    QModelIndex index;

    int separatorCount = 0;
    QMenu* menu = new QMenu(0);
    menu->setAttribute(Qt::WA_DeleteOnClose);

    QString cleanedPath = cleanPath(node.path());

    if (!cleanedPath.isEmpty() && !isValid(cleanedPath)) {
        return 0;
    }

    const QStringList sections = pathSections(node.path());

    if (node.type() != QIrBreadCrumbModelNode::Global) {
        QModelIndex parent = sApp->outlineModel()->index(sections);
        int count = sApp->outlineModel()->rowCount(parent);

        for (int i = 0; i < count; ++i) {
            index = sApp->outlineModel()->index(i, 0, parent);
            if (index.isValid()) {
                const QString title = index.data().toString();
                const QString childPath = cleanedPath + SEPARATOR + title;
                QAction* act = new QAction(icon(pathNodeType(QStringList() << sections << title)), title, menu);
                act->setData(childPath);
                menu->addAction(act);
            }
        }
    }
    else {
        int count = sections.size();

        for (int i = count - 1; i >= 0; --i) {
            const QString path = cleanPath(QStringList(sections.mid(0, i + 1)).join(SEPARATOR));
            const QString title = sections.at(i);
            QAction* act = new QAction(icon(i == 0 ? QIrBreadCrumbModelNode::Root : QIrBreadCrumbModelNode::Container), title, menu);
            act->setData(path);
            menu->addAction(act);
        }

        menu->addSeparator();
        ++separatorCount;

        const QStringList bookmarks = sApp->quickAccessBookmarks();

        for (int i = 0; i < bookmarks.size(); ++i) {
            const QString path = bookmarks.at(i);
            const QStringList sections = pathSections(path);
            const QString title = sections.isEmpty()
                                  ? SaagharWidget::rootTitle()
                                  : sections.size() == 1
                                  ? sections.at(0)
                                  : sections.first() + QLatin1String(": ") + sections.last();

            QAction* act = new QAction(icon(pathNodeType(sections)), title, menu);
            act->setData(path);
            menu->addAction(act);
        }

        menu->addSeparator();
        ++separatorCount;
        QAction* act = sApp->quickAccessCustomizeAction();
        act->setParent(menu);
        menu->addAction(act);
    }

    if (menu->actions().size() == separatorCount) {
        menu->clear();
        menu->deleteLater();
        menu = 0;
    }

    return menu;
}

QStringList BreadCrumbSaagharModel::pathSections(const QString &path) const
{
    return path.split(SEPARATOR, QString::SkipEmptyParts);
}

QIrBreadCrumbModelNode::Type BreadCrumbSaagharModel::pathNodeType(const QStringList &sections) const
{
    if (sections.isEmpty() || (sections.size() == 1 && sections.at(0).compare(SaagharWidget::rootTitle(), Qt::CaseInsensitive) == 0)) {
        return QIrBreadCrumbModelNode::Root;
    }
    else {
        bool ok;
        const QModelIndex ind = sApp->outlineModel()->index(sections, &ok);
        if (ok && sApp->outlineModel()->rowCount(ind) > 0) {
            return QIrBreadCrumbModelNode::Container;
        }
        else {
            return QIrBreadCrumbModelNode::Leaf;
        }
    }
}

QIcon BreadCrumbSaagharModel::icon(const QIrBreadCrumbModelNode::Type &type) const
{
    const static QIcon rootIcon(":/saagharmodel/images/root-node.png");
    const static QIcon containerIcon(":/saagharmodel/images/container-node.png");
    const static QIcon leafIcon(":/saagharmodel/images/leaf-node.png");

    switch (type) {
    case QIrBreadCrumbModelNode::Root:
        return rootIcon;
    case QIrBreadCrumbModelNode::Container:
        return containerIcon;
    case QIrBreadCrumbModelNode::Leaf:
        return leafIcon;
    default:
        break;
    }

    return QIcon();
}

QIR_END_NAMESPACE
