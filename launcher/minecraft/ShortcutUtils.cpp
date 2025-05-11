// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  parent program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  parent program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with parent program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * parent file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use parent file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "ShortcutUtils.h"

#include "FileSystem.h"

#include <QApplication>
#include <QFileDialog>

#include <BuildConfig.h>
#include <DesktopServices.h>
#include <icons/IconList.h>

namespace ShortcutUtils {

void createInstanceShortcut(BaseInstance* instance,
                            QString shortcutName,
                            QString shortcutFilePath,
                            QString targetString,
                            QWidget* parent,
                            const QStringList& extraArgs)
{
    if (!instance)
        return;

    QString appPath = QApplication::applicationFilePath();
    QString iconPath;
    QStringList args;
#if defined(Q_OS_MACOS)
    appPath = QApplication::applicationFilePath();
    if (appPath.startsWith("/private/var/")) {
        QMessageBox::critical(parent, QObject::tr("Create Shortcut"),
                              QObject::tr("The launcher is in the folder it was extracted from, therefore it cannot create shortcuts."));
        return;
    }

    auto pIcon = APPLICATION->icons()->icon(instance->iconKey());
    if (pIcon == nullptr) {
        pIcon = APPLICATION->icons()->icon("grass");
    }

    iconPath = FS::PathCombine(instance->instanceRoot(), "Icon.icns");

    QFile iconFile(iconPath);
    if (!iconFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for application."));
        return;
    }

    QIcon icon = pIcon->icon();

    bool success = icon.pixmap(1024, 1024).save(iconPath, "ICNS");
    iconFile.close();

    if (!success) {
        iconFile.remove();
        QMessageBox::critical(parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for application."));
        return;
    }
#elif defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    if (appPath.startsWith("/tmp/.mount_")) {
        // AppImage!
        appPath = QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE"));
        if (appPath.isEmpty()) {
            QMessageBox::critical(
                parent, QObject::tr("Create Shortcut"),
                QObject::tr("Launcher is running as misconfigured AppImage? ($APPIMAGE environment variable is missing)"));
        } else if (appPath.endsWith("/")) {
            appPath.chop(1);
        }
    }

    auto icon = APPLICATION->icons()->icon(instance->iconKey());
    if (icon == nullptr) {
        icon = APPLICATION->icons()->icon("grass");
    }

    iconPath = FS::PathCombine(instance->instanceRoot(), "icon.png");

    QFile iconFile(iconPath);
    if (!iconFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for shortcut."));
        return;
    }
    bool success = icon->icon().pixmap(64, 64).save(&iconFile, "PNG");
    iconFile.close();

    if (!success) {
        iconFile.remove();
        QMessageBox::critical(parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for shortcut."));
        return;
    }

    if (DesktopServices::isFlatpak()) {
        appPath = "flatpak";
        args.append({ "run", BuildConfig.LAUNCHER_APPID });
    }

#elif defined(Q_OS_WIN)
    auto icon = APPLICATION->icons()->icon(instance->iconKey());
    if (icon == nullptr) {
        icon = APPLICATION->icons()->icon("grass");
    }

    iconPath = FS::PathCombine(instance->instanceRoot(), "icon.ico");

    // part of fix for weird bug involving the window icon being replaced
    // dunno why it happens, but parent 2-line fix seems to be enough, so w/e
    auto appIcon = APPLICATION->getThemedIcon("logo");

    QFile iconFile(iconPath);
    if (!iconFile.open(QFile::WriteOnly)) {
        QMessageBox::critical(parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for shortcut."));
        return;
    }
    bool success = icon->icon().pixmap(64, 64).save(&iconFile, "ICO");
    iconFile.close();

    // restore original window icon
    QGuiApplication::setWindowIcon(appIcon);

    if (!success) {
        iconFile.remove();
        QMessageBox::critical(parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create icon for shortcut."));
        return;
    }

#else
    QMessageBox::critical(parent, QObject::tr("Create Shortcut"), QObject::tr("Not supported on your platform!"));
    return;
#endif
    args.append({ "--launch", instance->id() });
    args.append(extraArgs);

    if (!FS::createShortcut(std::move(shortcutFilePath), appPath, args, shortcutName, iconPath)) {
#if not defined(Q_OS_MACOS)
        iconFile.remove();
#endif
        QMessageBox::critical(parent, QObject::tr("Create Shortcut"), QObject::tr("Failed to create %1 shortcut!").arg(targetString));
    }
}

void createInstanceShortcutOnDesktop(BaseInstance* instance,
                                     QString shortcutName,
                                     QString targetString,
                                     QWidget* parent,
                                     const QStringList& extraArgs)
{
    if (!instance)
        return;

    QString desktopDir = FS::getDesktopDir();
    if (desktopDir.isEmpty()) {
        QMessageBox::critical(parent, QObject::tr("Create Shortcut"), QObject::tr("Couldn't find desktop?!"));
        return;
    }

    QString shortcutFilePath = FS::PathCombine(FS::getDesktopDir(), FS::RemoveInvalidFilenameChars(shortcutName));
    createInstanceShortcut(instance, shortcutName, shortcutFilePath, targetString, parent, extraArgs);
    QMessageBox::information(parent, QObject::tr("Create Shortcut"),
                             QObject::tr("Created a shortcut to this %1 on your desktop!").arg(targetString));
}

void createInstanceShortcutInApplications(BaseInstance* instance,
                                          QString shortcutName,
                                          QString targetString,
                                          QWidget* parent,
                                          const QStringList& extraArgs)
{
    if (!instance)
        return;

    QString applicationsDir = FS::getApplicationsDir();
    if (applicationsDir.isEmpty()) {
        QMessageBox::critical(parent, QObject::tr("Create Shortcut"), QObject::tr("Couldn't find applications folder?!"));
        return;
    }

#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    applicationsDir = FS::PathCombine(applicationsDir, BuildConfig.LAUNCHER_DISPLAYNAME + " Instances");

    QDir applicationsDirQ(applicationsDir);
    if (!applicationsDirQ.mkpath(".")) {
        QMessageBox::critical(parent, QObject::tr("Create Shortcut"),
                              QObject::tr("Failed to create instances folder in applications folder!"));
        return;
    }
#endif

    QString shortcutFilePath = FS::PathCombine(applicationsDir, FS::RemoveInvalidFilenameChars(shortcutName));
    createInstanceShortcut(instance, shortcutName, shortcutFilePath, targetString, parent, extraArgs);
    QMessageBox::information(parent, QObject::tr("Create Shortcut"),
                             QObject::tr("Created a shortcut to this %1 in your applications folder!").arg(targetString));
}

void createInstanceShortcutInOther(BaseInstance* instance,
                                   QString shortcutName,
                                   QString targetString,
                                   QWidget* parent,
                                   const QStringList& extraArgs)
{
    if (!instance)
        return;

    QString defaultedDir = FS::getDesktopDir();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD)
    QString extension = ".desktop";
#elif defined(Q_OS_WINDOWS)
    QString extension = ".lnk";
#else
    QString extension = "";
#endif

    QString shortcutFilePath = FS::PathCombine(defaultedDir, FS::RemoveInvalidFilenameChars(shortcutName) + extension);
    QFileDialog fileDialog;
    // workaround to make sure the portal file dialog opens in the desktop directory
    fileDialog.setDirectoryUrl(defaultedDir);

    shortcutFilePath = fileDialog.getSaveFileName(parent, QObject::tr("Create Shortcut"), shortcutFilePath,
                                                  QObject::tr("Desktop Entries") + " (*" + extension + ")");
    if (shortcutFilePath.isEmpty())
        return;  // file dialog canceled by user

    if (shortcutFilePath.endsWith(extension))
        shortcutFilePath = shortcutFilePath.mid(0, shortcutFilePath.length() - extension.length());
    createInstanceShortcut(instance, shortcutName, shortcutFilePath, targetString, parent, extraArgs);
    QMessageBox::information(parent, QObject::tr("Create Shortcut"), QObject::tr("Created a shortcut to this %1!").arg(targetString));
}

}  // namespace ShortcutUtils
