/*
 * LinuxUserFunctions.cpp - implementation of LinuxUserFunctions class
 *
 * Copyright (c) 2017 Tobias Junghans <tobydox@users.sf.net>
 *
 * This file is part of Veyon - http://veyon.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include <QProcess>

#include "LinuxUserFunctions.h"

#include <pwd.h>
#include <unistd.h>


QString LinuxUserFunctions::fullName( const QString& username )
{
	auto pw_entry = getpwnam( VeyonCore::stripDomain( username ).toUtf8().constData() );

	if( pw_entry )
	{
		QString shell( pw_entry->pw_shell );

		// Skip not real users
		if ( !( shell.endsWith( QStringLiteral( "/false" ) ) ||
		        shell.endsWith( QStringLiteral( "/true" ) ) ||
		        shell.endsWith( QStringLiteral( "/null" ) ) ||
		        shell.endsWith( QStringLiteral( "/nologin" ) ) ) )
		{
			return QString::fromUtf8( pw_entry->pw_gecos ).split( ',' ).first();
		}
	}

	return QString();
}



QStringList LinuxUserFunctions::userGroups( bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups);

	QStringList groupList;

	QProcess getentProcess;
	getentProcess.start( QStringLiteral("getent"), { QStringLiteral("group") } );
	getentProcess.waitForFinished();

	const auto groups = QString( getentProcess.readAll() ).split( '\n' );

	groupList.reserve( groups.size() );

	for( const auto& group : groups )
	{
		groupList += group.split( ':' ).first();
	}

	const QStringList ignoredGroups( {
	    "root",
	    "daemon",
	    "bin",
	    "tty",
	    "disk",
	    "lp",
	    "mail",
	    "news",
	    "uucp",
	    "man",
	    "proxy",
	    "kmem",
	    "dialout",
	    "fax",
	    "voice",
	    "cdrom",
	    "tape",
	    "audio",
	    "dip",
	    "www-data",
	    "backup",
	    "list",
	    "irc",
	    "src",
	    "gnats",
	    "shadow",
	    "utmp",
	    "video",
	    "sasl",
	    "plugdev",
	    "games",
	    "users",
	    "nogroup",
	    "libuuid",
	    "syslog",
	    "fuse",
	    "lpadmin",
	    "ssl-cert",
	    "messagebus",
	    "crontab",
	    "mlocate",
	    "avahi-autoipd",
	    "netdev",
	    "saned",
	    "sambashare",
	    "haldaemon",
	    "polkituser",
	    "mysql",
	    "avahi",
	    "klog",
	    "floppy",
	    "oprofile",
	    "netdev",
	    "dirmngr",
	    "vboxusers",
	    "bluetooth",
	    "colord",
	    "libvirtd",
	    "nm-openvpn",
	    "input",
	    "kvm",
	    "pulse",
	    "pulse-access",
	    "rtkit",
	    "scanner",
	    "sddm",
	    "systemd-bus-proxy",
	    "systemd-journal",
	    "systemd-network",
	    "systemd-resolve",
	    "systemd-timesync",
	    "uuidd",
	                           } );

	for( const auto& ignoredGroup : ignoredGroups )
	{
		groupList.removeAll( ignoredGroup );
	}

	// remove all empty entries
	groupList.removeAll( QStringLiteral("") );

	return groupList;
}



QStringList LinuxUserFunctions::groupsOfUser( const QString& username, bool queryDomainGroups )
{
	Q_UNUSED(queryDomainGroups);

	QStringList groupList;

	const auto strippedUsername = VeyonCore::stripDomain( username );

	QProcess getentProcess;
	getentProcess.start( QStringLiteral("getent"), { QStringLiteral("group") } );
	getentProcess.waitForFinished();

	const auto groups = QString( getentProcess.readAll() ).split( '\n' );
	for( const auto& group : groups )
	{
		const auto groupComponents = group.split( ':' );
		if( groupComponents.size() == 4 &&
		        groupComponents.last().split( ',' ).contains( strippedUsername ) )
		{
			groupList += groupComponents.first(); // clazy:exclude=reserve-candidates
		}
	}

	groupList.removeAll( QStringLiteral("") );

	return groupList;
}



QString LinuxUserFunctions::currentUser()
{
	QString username;

	char * envUser = getenv( "USER" );

	struct passwd * pw_entry = nullptr;
	if( envUser )
	{
		pw_entry = getpwnam( envUser );
	}

	if( !pw_entry )
	{
		pw_entry = getpwuid( getuid() );
	}

	if( pw_entry )
	{
		QString shell( pw_entry->pw_shell );

		// Skip not real users
		if ( !( shell.endsWith( QStringLiteral( "/false" ) ) ||
		        shell.endsWith( QStringLiteral( "/true" ) ) ||
		        shell.endsWith( QStringLiteral( "/null" ) ) ||
		        shell.endsWith( QStringLiteral( "/nologin" ) ) ) )
		{
			username = QString::fromUtf8( pw_entry->pw_name );
		}
	}

	if( username.isEmpty() )
	{
		return QString::fromUtf8( envUser );
	}

	return username;
}



QStringList LinuxUserFunctions::loggedOnUsers()
{
	QStringList users;

	QProcess whoProcess;
	whoProcess.start( QStringLiteral("who") );
	whoProcess.waitForFinished( WhoProcessTimeout );

	if( whoProcess.exitCode() != 0 )
	{
		return users;
	}

	const auto lines = whoProcess.readAll().split( '\n' );
	for( const auto& line : lines )
	{
		const auto user = line.split( ' ' ).value( 0 );
		if( user.isEmpty() == false && users.contains( user ) == false )
		{
			users.append( user ); // clazy:exclude=reserve-candidates
		}
	}

	return users;
}



void LinuxUserFunctions::logon( const QString& username, const QString& password )
{
	Q_UNUSED(username);
	Q_UNUSED(password);

	// TODO
}



void LinuxUserFunctions::logout()
{
	// Gnome logout, 2 = forced mode (don't wait for unresponsive processes)
	QProcess::startDetached( QStringLiteral( "dbus-send --session --dest=org.gnome.SessionManager --type=method_call /org/gnome/SessionManager org.gnome.SessionManager.Logout uint32:2" ) );
	// KDE 4 logout
	QProcess::startDetached( QStringLiteral( "qdbus org.kde.ksmserver /KSMServer logout 0 0 0" ) );
	// KDE 5 logout
	QProcess::startDetached( QStringLiteral( "dbus-send --dest=org.kde.ksmserver /KSMServer org.kde.KSMServerInterface.logout int32:0 int32:2 int32:0" ) );
	// Xfce logout
	QProcess::startDetached( QStringLiteral("xfce4-session-logout --logout") );
}



bool LinuxUserFunctions::authenticate( const QString& username, const QString& password )
{
	QProcess p;
	p.start( QStringLiteral( "veyon-auth-helper" ) );
	p.waitForStarted();

	QDataStream ds( &p );
	ds << VeyonCore::stripDomain( username );
	ds << password;

	p.closeWriteChannel();
	p.waitForFinished();

	if( p.exitCode() != 0 )
	{
		qCritical() << "VeyonAuthHelper failed:" << p.readAll().trimmed();
		return false;
	}

	qDebug( "User authenticated successfully" );
	return true;
}
