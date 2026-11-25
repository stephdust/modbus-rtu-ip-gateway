// Native cursor test: g++ -std=c++17 -Wall -Wextra -Werror -pedantic
// tests/test_frame_delta_policy.cpp -o /tmp/test_frame_delta && /tmp/test_frame_delta
#include "../components/modbus_gateway/frame_delta_policy.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
using namespace frame_delta_policy;
int main() {
  unsigned long cases=0;
  auto check=[&](uint32_t newest,size_t retained,size_t limit,bool supplied,
                 uint32_t after,bool same_epoch,size_t n,bool reset) {
    const Selection s=select(newest,retained,limit,supplied,after,same_epoch);
    assert(s.count==n && s.reset==reset);
    ++cases;
  };
  check(0,0,64,false,0,false,0,true);
  check(64,64,64,false,0,false,64,true);
  check(65,65,64,true,64,true,1,false);
  check(65,65,64,true,65,true,0,false);
  check(65,65,64,true,1,true,64,false);
  check(65,65,64,true,0,true,64,true);
  check(1234,256,64,true,1000,true,64,true);
  check(100,10,64,true,92,true,8,false);
  check(100,10,64,true,89,true,10,true);
  check(0,0,64,true,999,false,0,true);
  check(42,12,64,true,42,false,12,true);
  check(1,1,64,true,2,true,1,true);
  check(50,256,64,true,50,true,0,false);
  for(uint32_t latest=1;latest<400;++latest)
    for(size_t retained=1;retained<=64;++retained)
      for(uint32_t delta=0;delta<100;++delta) {
        const Selection x=select(latest,retained,64,true,latest-delta,true);
        assert(x.reset==(delta>retained));
        assert(x.count==((delta>retained)?retained:delta));
        ++cases;
      }
  uint32_t result=0U;
  assert(parse_u32("0",result)&&result==0U);
  assert(parse_u32("4294967295",result)&&result==UINT32_MAX);
  for(const char *invalid: {"","-1","4294967296","+1","12x"," 3","1.0"}) {
    assert(!parse_u32(invalid,result));
    ++cases;
  }
  std::printf("PASS: %lu cursor selection/catch-up/reset/uint32 cases\n",cases);
  return 0;
}
