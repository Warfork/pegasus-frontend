// Pegasus Frontend
// Copyright (C) 2017  Mátyás Mustoha
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.


#include "SteamPlatform.h"

#include "model/Platform.h"

#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStringBuilder>

static constexpr auto MSG_PREFIX = "Steam:";


namespace {

QString find_steam_datadir()
{
    using Paths = QStandardPaths;
    const auto steam_subdir = QLatin1String("/Steam/");
    const auto possible_basedirs = Paths::standardLocations(Paths::GenericDataLocation);

    for (const auto& basedir : possible_basedirs) {
        const QString steamdir_maybe = basedir % steam_subdir;
        if (QFileInfo(steamdir_maybe).isDir()) {
            qInfo().noquote() << MSG_PREFIX
                              << QObject::tr("found data directory: `%1`").arg(steamdir_maybe);
            return steamdir_maybe;
        }
    }

    return QString();
}

QStringList find_steam_installdirs(const QString& steam_datadir)
{
    QStringList installdirs;
    installdirs << steam_datadir % QLatin1String("steamapps");


    const QString config_path = steam_datadir % QLatin1String("config/config.vdf");
    QFile configfile(config_path);
    if (!configfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning().noquote() << MSG_PREFIX
            << QObject::tr("while Steam seems to be installed, "
                           "the config file `%1` could not be opened").arg(config_path);
        return installdirs;
    }


    const QRegularExpression installdir_regex(R""("BaseInstallFolder_\d+"\s+"([^"]+)")"");

    QTextStream stream(&configfile);
    while (!stream.atEnd()) {
        const QString line = stream.readLine();
        const auto match = installdir_regex.match(line);
        if (match.hasMatch()) {
            const QString path = match.captured(1) % QLatin1String("/steamapps");
            if (!installdirs.contains(path))
                installdirs << path;
        }
    }

    return installdirs;
}

} // namespace


namespace model_providers {

SteamPlatform::SteamPlatform()
{
}

QVector<Model::Platform*> SteamPlatform::find()
{
    const QString steamdir = find_steam_datadir();
    if (steamdir.isEmpty())
        return {};

    QStringList installdirs = find_steam_installdirs(steamdir);
    installdirs.erase(std::remove_if(
        installdirs.begin(), installdirs.end(),
            [](const QString& dir){ return !QFileInfo(dir).isDir(); }),
        installdirs.end());
    if (installdirs.isEmpty()) {
        qWarning().noquote() << MSG_PREFIX << QObject::tr("no installation directories found");
        return {};
    }

    QVector<Model::Platform*> result;
    result.push_back(new Model::Platform(
        QLatin1String("steam"),
        installdirs,
        QStringList(QLatin1String("appmanifest_*.acf"))
    ));
    return result;
}

} // namespace model_providers
