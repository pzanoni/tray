#!/bin/bash

if [ -n "$MANDRIVA_MINI_SESSION" ]; then
	kill $MANDRIVA_MINI_SESSION
elif [ -n "$KDE_FULL_SESSION" ]; then
	qdbus org.kde.ksmserver /KSMServer org.kde.KSMServerInterface.logout 0 0 0
elif ps x | grep gnome-session | grep -v grep; then
	/usr/bin/gnome-session-save --kill --silent
else
    kill -9 -1
fi
