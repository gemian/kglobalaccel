/* Copyright (C) 2008 Michael Jansen <kde@michael-jansen.biz>
   Copyright (C) 2016 Marco Martin <mart@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kserviceactioncomponent.h"
#include "globalshortcutcontext.h"
#include "logging_p.h"

#include <QProcess>
#include <QDBusConnectionInterface>


namespace KdeDGlobalAccel {

KServiceActionComponent::KServiceActionComponent(
            const QString &serviceStorageId,
            const QString &friendlyName,
            GlobalShortcutsRegistry *registry)
    :   Component(serviceStorageId, friendlyName, registry),
        m_serviceStorageId(serviceStorageId),
        m_desktopFile(QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("kglobalaccel/") + serviceStorageId))
    {
    }


KServiceActionComponent::~KServiceActionComponent()
    {
    }


void runProcess(const KConfigGroup &group)
{
    auto parts = group.readEntry(QStringLiteral("Exec"), QString()).split(QChar(' '));
    if (parts.isEmpty()) {
        return;
    }
    //sometimes entries have an %u for command line parameters
    if (parts.last().contains(QChar('%'))) {
        parts.pop_back();
    }

    const auto kstart = QStandardPaths::findExecutable(QStringLiteral("kstart5"));
    if (kstart.isEmpty()) {
        const QString command = parts.takeFirst();
        QProcess::startDetached(command, parts);
    } else {
        QProcess::startDetached(kstart, parts);
    }
}

void KServiceActionComponent::emitGlobalShortcutPressed( const GlobalShortcut &shortcut )
{
    //we can't use KRun there as it depends from KIO and would create a circular dep
    if (shortcut.uniqueName() == QLatin1String("_launch")) {
        runProcess(m_desktopFile.desktopGroup());
        return;
    }
    const auto lstActions = m_desktopFile.readActions();
    for (const QString &action : lstActions) {
        if (action == shortcut.uniqueName()) {
            runProcess(m_desktopFile.actionGroup(action));
            return;
        }
    }
}



void KServiceActionComponent::loadFromService()
    {

    QString shortcutString;

    QStringList shortcuts = m_desktopFile.desktopGroup().readEntry(QStringLiteral("X-KDE-Shortcuts"), QString()).split(QChar(','));
    if (!shortcuts.isEmpty()) {
        shortcutString = shortcuts.join(QChar('\t'));
    }

    GlobalShortcut *shortcut = registerShortcut(QStringLiteral("_launch"), m_desktopFile.readName(), shortcutString, shortcutString);
    shortcut->setIsPresent(true);
    const auto lstActions = m_desktopFile.readActions();
    for (const QString &action : lstActions)
        {
        shortcuts = m_desktopFile.actionGroup(action).readEntry(QStringLiteral("X-KDE-Shortcuts"), QString()).split(QChar(','));
        if (!shortcuts.isEmpty())
            {
            shortcutString = shortcuts.join(QChar('\t'));
            }

        GlobalShortcut *shortcut = registerShortcut(action, m_desktopFile.actionGroup(action).readEntry(QStringLiteral("Name")), shortcutString, shortcutString);
        shortcut->setIsPresent(true);
        }
    }

bool KServiceActionComponent::cleanUp()
{
    qCDebug(KGLOBALACCELD) << "Disabling desktop file";

    const auto shortcuts = allShortcuts();
    for (GlobalShortcut *shortcut : shortcuts) {
        shortcut->setIsPresent(false);
    }

    m_desktopFile.desktopGroup().writeEntry("NoDisplay", true);
    m_desktopFile.desktopGroup().sync();

    return Component::cleanUp();
}

} // namespace KdeDGlobalAccel

#include "moc_kserviceactioncomponent.cpp"
