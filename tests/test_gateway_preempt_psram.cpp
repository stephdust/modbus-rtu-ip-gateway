// Native C++17 policy and fixed frame formatting tests. No ESPHome required.
// g++ -std=c++17 -Wall -Wextra -Werror -pedantic tests/test_gateway_preempt_psram.cpp -o /tmp/gateway_fix && /tmp/gateway_fix
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <array>
#include "../components/modbus_gateway/deferred_rtu_policy.h"
#include "../components/modbus_gateway/frame_log_policy.h"
int main() {
  using namespace deferred_rtu_policy;
  unsigned cases = 0;
  for(int bg=0; bg<2; ++bg)
    for(int act=0; act<2; ++act)
      for(int hit=0; hit<2; ++hit)
        for(int fc=1; fc<24; ++fc) {
          const bool expected=bg && act && !hit && fc<=4;
          assert(should_defer(act,bg,hit,uint8_t(fc))==expected);
          ++cases;
        }
  for(int f=1; f<24; ++f)
    for(uint32_t age=0; age<1500000U; age+=997U) {
      assert(read_expired(uint8_t(f), age, 700U)==(f<=4 && age/1000U>=700U));
      assert(discard_for_new_master(uint8_t(f),true)==(f<=4));
      ++cases;
    }
  for(int active=0; active<2; ++active)
    for(int idle=0; idle<2; ++idle) {
      assert(can_dispatch(active,idle)==(active&&idle));
      ++cases;
    }
  std::array<uint8_t,300> data{};
  for(size_t i=0;i<data.size();++i)data[i]=uint8_t(i);
  for(size_t n=0;n<=data.size();++n) {
    char out[784]; memset(out,'?',sizeof(out));
    const size_t written=frame_log_policy::encode_hex(out,sizeof(out),data.data(),n);
    assert(written<sizeof(out)); assert(out[written]=='\0');
    if(n<=260U) {
      assert(strlen(out)==(n ? 3U*n-1U : 0U));
      assert(strstr(out," ...")==nullptr);
    }
    if(n>261U) assert(strlen(out)<sizeof(out));
    ++cases;
  }
  {
    char out[4]; const uint8_t byte=0xAA;
    assert(frame_log_policy::encode_hex(out,sizeof(out),&byte,1U)==2U);
    assert(strcmp(out,"AA")==0);
    assert(frame_log_policy::encode_hex(out,0U,&byte,1U)==0U);
    assert(frame_log_policy::encode_hex(nullptr,4U,&byte,1U)==0U);
    assert(frame_log_policy::encode_hex(out,4U,nullptr,1U)==0U);
  }
  puts("PASS: 35104 deferred-policy/frame-encoding cases");
  return 0;
}
