#!/bin/bash

APPNAME=FET

sed -e "/^GenericName/d" -e "s/^Name/GenericName/" -e "/^Type=/ a Name=$APPNAME" share/applications/fet.desktop > fet.desktop

# In place:
#sed -i -e "/^GenericName/d" -e "s/^Name/GenericName/" -e "/^Type=/ a Name=$APPNAME" ../fet.desktop

