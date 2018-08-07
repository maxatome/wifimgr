# FreeBSD wifimgr

Add some features to [wifimgr-1.11](http://opal.com/freebsd/ports/net-mgmt/wifimgr/):

- new button "Hide unavailable"/"Show all" button to display only
  available networks or available+recorded networks (by default
  displays available+recorded networks);
- can now order networks by SSID (default), signal strength or
  channel;
- `wifimgr` accepts some options:
  - `-o sorting-criteria`, `--order-by sorting-criteria`
    - `sorting-criteria` defaults to `ssid` but can be `signal` or `channel`,
  - `-H`, `--hide-unavailable`
	- show only available networks,
- correct some clang warnings.
