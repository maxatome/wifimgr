# FreeBSD wifimgr

Add some features to [wifimgr](http://opal.com/freebsd/ports/net-mgmt/wifimgr/):

- new "Show all Networks" check box to display only visible networks or
  visible+recorded networks;
- by default displays visible+recorded networks;
- can now order networks by SSID (default), signal strength or
  channel;
- `wifimgr` accepts some options:
  - `-o sorting-criteria`, `--order-by sorting-criteria`
    - `sorting-criteria` defaults to `ssid` but can be `signal` or `channel`,
  - `-s`, `--dont-show-all`
	- show only visible networks,
- correct some clang warnings.
