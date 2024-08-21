// v4: separate "time spent in the software stack" from everything else by
// comparing a NIC (or kernel) receive timestamp with the application's rdtsc
// when it actually sees the packet.
//
//   software-stack latency = t_app_after_recvmsg - t_nic_rx
//
// Linux only. Uses SO_TIMESTAMPING. Hardware timestamps need a NIC + driver that
// support them (check `ethtool -T <iface>`); otherwise this falls back to the
// kernel's software RX timestamp and says so. On loopback only software
// timestamps exist. NIC clocks are usually PTP-domain time, not CLOCK_REALTIME:
// compare NIC and app clocks only after syncing them (phc2sys / ptp4l), or
// look at differences between consecutive packets, which cancel a constant offset.
//
// usage: nic_timestamps [port=9999] [count=1000]      (sends to itself on 127.0.0.1)
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

