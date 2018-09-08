# FreeBSD wifimgr

**All the following changes have been integrated in wifimgr-1.12, except Schift-click1 feature replaced by new behavior of "Any BSSID" (folding of all same-SSID networks into one line).**

Add some features to [wifimgr-1.11](http://opal.com/freebsd/ports/net-mgmt/wifimgr/):

- Shift-click-1 on a network checkbox select/unselect all networks with
  the same SSID;
- new button "Hide unavailable"/"Show all" button to display only
  available networks or available+recorded networks (by default
  displays available+recorded networks);
- can now order networks by SSID (default), signal strength or
  channel;
- `wifimgr` automatically loads/saves user choices in `~/.wifimgr`;
- correct some clang warnings.
