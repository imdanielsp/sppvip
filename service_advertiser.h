#ifndef SERVICE_ADVERTISER_h
#define SERVICE_ADVERTISER_h

#include <memory>
#include <thread>

#include <dns_sd.h>

namespace sppvip {

class service_advertiser
{
public:
    service_advertiser();

    ~service_advertiser();

    bool is_advertising() const noexcept { return is_advertising_; }

    bool advertise() noexcept;

    void stop();

private:
    bool is_advertising_;
    DNSServiceRef sppvip_reg_srv_;
    std::unique_ptr<std::thread> advertise_thread_;
    std::mutex advertise_mutex_;
};
};
#endif
