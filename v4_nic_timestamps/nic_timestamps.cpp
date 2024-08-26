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

#if defined(__linux__)
#include <arpa/inet.h>
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <vector>

static double ts_ns(const timespec& t) { return t.tv_sec * 1e9 + t.tv_nsec; }
static double realtime_ns() { timespec t; clock_gettime(CLOCK_REALTIME, &t); return ts_ns(t); }

int main(int argc, char** argv) {
  int port = argc > 1 ? std::atoi(argv[1]) : 9999, count = argc > 2 ? std::atoi(argv[2]) : 1000;
  int rx = socket(AF_INET, SOCK_DGRAM, 0), tx = socket(AF_INET, SOCK_DGRAM, 0);
  sockaddr_in a{};
  a.sin_family = AF_INET; a.sin_port = htons(port); a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  if (bind(rx, (sockaddr*)&a, sizeof a) < 0) { std::perror("bind"); return 1; }

  int flags = SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE |
              SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_SOFTWARE;
  if (setsockopt(rx, SOL_SOCKET, SO_TIMESTAMPING, &flags, sizeof flags) < 0) {
    std::perror("SO_TIMESTAMPING"); return 1;
  }
  std::vector<double> stack_ns;
  bool saw_hw = false;
  for (int i = 0; i < count; ++i) {
    char msg[16] = "ping";
    sendto(tx, msg, sizeof msg, 0, (sockaddr*)&a, sizeof a);
    char data[64], ctrl[256];
    iovec iov{data, sizeof data};
    msghdr mh{};
    mh.msg_iov = &iov; mh.msg_iovlen = 1; mh.msg_control = ctrl; mh.msg_controllen = sizeof ctrl;
    if (recvmsg(rx, &mh, 0) < 0) { std::perror("recvmsg"); return 1; }
    double t_app = realtime_ns();
    for (cmsghdr* c = CMSG_FIRSTHDR(&mh); c; c = CMSG_NXTHDR(&mh, c)) {
      if (c->cmsg_level == SOL_SOCKET && c->cmsg_type == SCM_TIMESTAMPING) {
        timespec* ts = (timespec*)CMSG_DATA(c);  // [0]=software, [2]=raw hardware
        double t_nic = ts_ns(ts[2]) > 0 ? ts_ns(ts[2]) : ts_ns(ts[0]);
        if (ts_ns(ts[2]) > 0) saw_hw = true;
        stack_ns.push_back(t_app - t_nic);
      }
    }
  }
  if (stack_ns.empty()) { std::puts("no timestamps returned by kernel"); return 1; }
  std::sort(stack_ns.begin(), stack_ns.end());
  auto at = [&](double q) { return stack_ns[(size_t)(q * (stack_ns.size() - 1))]; };
  std::printf("source: %s\n", saw_hw ? "NIC hardware timestamp" : "kernel software timestamp (no HW ts available)");
  std::printf("rx->app  p50=%.0f ns  p99=%.0f ns  max=%.0f ns  (n=%zu)\n", at(0.5), at(0.99), stack_ns.back(), stack_ns.size());
}
#else
int main() { std::puts("v4 is Linux only (SO_TIMESTAMPING)."); }
#endif
